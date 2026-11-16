#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace bass {
constexpr double pi = 3.14159265358979323846;
inline float gain(float db) { return std::pow(10.0f, db * 0.05f); }
struct Settings {
    // Input, bass, treble, compression, sub, distortion, pitch, output, balance.
    std::array<float, 9> faders{};
    std::array<int, 6> colors{1, 1, 1, 1, 1, 1};
    bool bypass = false;
};
struct Biquad {
    double b0=1, b1=0, b2=0, a1=0, a2=0, z1=0, z2=0;
    float tick(float x) {
        const double y=b0*x+z1; z1=b1*x-a1*y+z2; z2=b2*x-a2*y;
        return static_cast<float>(y);
    }
    void peak(double rate, double freq, double q, double db) {
        const double a=std::pow(10.,db/40.), w=2*pi*freq/rate;
        const double alpha=std::sin(w)/(2*q), c=std::cos(w), d=1+alpha/a;
        b0=(1+alpha*a)/d; b1=-2*c/d; b2=(1-alpha*a)/d;
        a1=-2*c/d; a2=(1-alpha/a)/d;
    }
    void reset() { z1=z2=0; }
};
class Engine {
public:
    void prepare(double sampleRate) {
        rate=std::clamp(sampleRate, 8000., 384000.);
        delaySize=static_cast<int>(rate*0.10)+4;
        for(auto& d:delay) d.assign(static_cast<size_t>(delaySize),0);
        smoothCoeff=static_cast<float>(1-std::exp(-1/(rate*0.02)));
        hpCoeff=static_cast<float>(std::exp(-2*pi*22/rate));
        lowCoeff=static_cast<float>(1-std::exp(-2*pi*110/rate));
        subEnvCoeff=static_cast<float>(1-std::exp(-1/(rate*0.03)));
        reset();
    }
    void reset() {
        for(auto& f:eqLow) f.reset();
        for(auto& f:eqHigh) f.reset();
        for(auto& d:delay) std::fill(d.begin(),d.end(),0);
        hpX={}; hpY={}; low={}; subLow={}; env=0; subEnv=0;
        previous=0; octave=1; phase=0; write=0; counter=0; initial=true;
        inputPeak={}; outputPeak={};
    }
    void process(float* left, float* right, int samples, const Settings& requested) {
        if(samples<=0 || delaySize==0) return;
        Settings s=requested;
        for(auto& f:s.faders) f=std::isfinite(f)? std::clamp(f,-10.f,10.f):0.f;
        s.faders[8]=std::clamp(s.faders[8],-6.f,6.f);
        for(size_t c=0;c<6;++c) s.colors[c]=std::clamp(s.colors[c],0,c==3?1:3);
        if(initial) { smoothed=s.faders; initial=false; }
        inputPeak={}; outputPeak={};
        const int cm=std::max(0,s.colors[2]-1);
        const double attacks[]={0.025,0.008,0.001}, releases[]={0.18,0.09,0.05};
        const float attack=static_cast<float>(std::exp(-1/(rate*attacks[cm])));
        const float release=static_cast<float>(std::exp(-1/(rate*releases[cm])));
        for(int i=0;i<samples;++i) {
            const float raw[2]={std::isfinite(left[i])?left[i]:0.f, std::isfinite(right[i])?right[i]:0.f};
            for(size_t c=0;c<9;++c) smoothed[c]+=smoothCoeff*(s.faders[c]-smoothed[c]);
            if((counter++ & 31)==0) {
                const double lf[]={60,100,220}, hf[]={900,2000,4500};
                for(size_t c=0;c<2;++c) {
                    eqLow[c].peak(rate,lf[std::max(0,s.colors[0]-1)],0.7,s.colors[0]?smoothed[1]:0);
                    eqHigh[c].peak(rate,hf[std::max(0,s.colors[1]-1)],0.65,s.colors[1]?smoothed[2]:0);
                }
            }
            float x[2];
            for(size_t c=0;c<2;++c) {
                const float g=gain(smoothed[0] + (c==0 ? -std::max(0.f,smoothed[8]) : std::min(0.f,smoothed[8])));
                x[c]=raw[c]*g;
                inputPeak[c]=std::max(inputPeak[c],std::abs(x[c]));
                const float hp=x[c]-hpX[c]+hpCoeff*hpY[c]; hpX[c]=x[c]; hpY[c]=hp;
                x[c]=eqHigh[c].tick(eqLow[c].tick(hp));
            }
            // Stereo-linked peak detector: channels retain their relative balance.
            const float peak=std::max(std::abs(x[0]),std::abs(x[1]));
            const float ec=peak>env?attack:release;
            env=ec*env+(1-ec)*peak;
            float cg=1;
            if(s.colors[2]) {
                const float ratios[]={2,4,12};
                const float threshold=-20-smoothed[3]*0.8f;
                const float over=std::max(0.f,20*std::log10(std::max(env,1.e-9f))-threshold);
                cg=gain(-over*(1-1/ratios[cm])+3+smoothed[3]*0.15f);
            }
            for(auto& a:x) a*=cg;
            const float mono=(x[0]+x[1])*0.5f;
            subEnv+=subEnvCoeff*(std::abs(mono)-subEnv);
            if(previous<=0 && mono>0 && subEnv>0.0001f) octave=-octave;
            previous=mono;
            for(size_t c=0;c<2;++c) {
                low[c]+=lowCoeff*(x[c]-low[c]);
                subLow[c]+=lowCoeff*(octave*subEnv-subLow[c]);
                float out=x[c];
                if(s.colors[3]) out+=(low[c]*0.25f+subLow[c]*0.65f)*gain(smoothed[4]-7);
                if(s.colors[4]) {
                    const float drive[]={2,6,14};
                    const float d=drive[s.colors[4]-1];
                    // Parallel high-passed saturation preserves the fundamental.
                    const float wet=std::tanh((x[c]-low[c])*d)/std::sqrt(d);
                    out+=wet*gain(smoothed[5]-12);
                }
                delay[c][static_cast<size_t>(write)]=out;
                if(s.colors[5]) {
                    const int mode=s.colors[5]-1;
                    const double base[]={0.018,0.012,0.008}, depth[]={0.003,0.004,0.0015};
                    const double offset=c==0?0:pi*(mode==0?0.5:1.0);
                    const double distance=rate*(base[mode]+depth[mode]*std::sin(phase+offset));
                    double read=write-distance; if(read<0) read+=delaySize;
                    const auto a=static_cast<int>(read); const float frac=static_cast<float>(read-a);
                    const float wet=delay[c][static_cast<size_t>(a)]*(1-frac)+delay[c][static_cast<size_t>((a+1)%delaySize)]*frac;
                    out+=wet*gain(smoothed[6]-12);
                }
                // Fixed soft overload control remains active when all Colors are clear.
                out=out/(1+0.08f*std::abs(out));
                out*=gain(smoothed[7]);
                if(s.bypass) out=raw[c];
                (c==0?left:right)[i]=out;
                outputPeak[c]=std::max(outputPeak[c],std::abs(out));
            }
            write=(write+1)%delaySize;
            phase+=2*pi*0.35/rate; if(phase>=2*pi) phase-=2*pi;
        }
    }
    std::array<float,2> inputPeak{},outputPeak{};
private:
    double rate=48000,phase=0;
    int delaySize=0,write=0; unsigned counter=0;
    float smoothCoeff=0,hpCoeff=0,lowCoeff=0,subEnvCoeff=0,env=0,subEnv=0,previous=0,octave=1;
    bool initial=true;
    std::array<float,9> smoothed{};
    std::array<float,2> hpX{},hpY{},low{},subLow{};
    std::array<Biquad,2> eqLow,eqHigh;
    std::array<std::vector<float>,2> delay;
};
}
