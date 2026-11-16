#include "BassDsp.h"
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

namespace {
int checks=0;
void require(bool condition,const std::string& why) {
    ++checks; if(!condition) throw std::runtime_error(why);
}
struct Signal {
    std::vector<float> l,r;
    Signal(int n=48000):l(static_cast<size_t>(n)),r(l.size()) {}
    void sine(double hz,double sr=48000,float amplitude=0.15f) {
        for(size_t i=0;i<l.size();++i) l[i]=r[i]=amplitude*static_cast<float>(std::sin(2*bass::pi*hz*static_cast<double>(i)/sr));
    }
    void run(bass::Engine& e,const bass::Settings& s,int block=257) {
        for(size_t i=0;i<l.size();i+=static_cast<size_t>(block))
            e.process(l.data()+i,r.data()+i,static_cast<int>(std::min(static_cast<size_t>(block),l.size()-i)),s);
    }
    double rms() const {double v=0; for(size_t i=l.size()/2;i<l.size();++i) v+=l[i]*l[i]; return std::sqrt(v/(l.size()-l.size()/2));}
};
bass::Settings clear() {bass::Settings s; s.colors.fill(0); return s;}
double tone(bass::Settings s,double hz) {bass::Engine e; e.prepare(48000); Signal a; a.sine(hz); a.run(e,s); return a.rms();}
}
int main() {
    try {
        for(double sr:{44100.,48000.,96000.,192000.}) {
            bass::Engine e; e.prepare(sr); Signal silent;
            silent.run(e,{});
            require(silent.rms()==0,"Default silence must stay silent");
            Signal a; a.sine(110,sr); auto original=a;
            auto bypass=clear(); bypass.bypass=true; a.run(e,bypass);
            require(a.l==original.l && a.r==original.r,"Global bypass must be bit-exact");
            std::mt19937 rng(21); std::uniform_real_distribution<float> random(-2,2);
            for(auto& x:a.l) x=random(rng);
            a.r=a.l;
            bass::Settings extreme; extreme.faders.fill(10);
            for(int mode=0;mode<4;++mode) {
                extreme.colors.fill(mode); a.run(e,extreme);
                for(auto x:a.l) require(std::isfinite(x)&&std::abs(x)<100,"Extreme input: bounded finite output");
                for(auto x:a.r) require(std::isfinite(x)&&std::abs(x)<100,"Right channel: bounded finite output");
            }
        }
        auto s=clear(); const double reference=tone(s,80);
        for(int color=1;color<4;++color) {
            s.colors[0]=color;
            require(std::abs(tone(s,80)-reference)<1.e-6,"EQ colors must be neutral at zero");
        }
        s=clear(); s.colors[0]=1; s.faders[1]=10;
        require(tone(s,60)>tone(clear(),60)*1.8,"Bass boost must raise low-frequency energy");
        s.faders[1]=-10;
        require(tone(s,60)<tone(clear(),60)*0.7,"Bass cut must reduce low-frequency energy");
        s=clear(); s.colors[1]=3; s.faders[2]=10;
        require(tone(s,4500)>tone(clear(),4500)*1.8,"Treble must boost its target band");
        s=clear(); s.faders[7]=-6;
        require(std::abs(tone(s,110)/tone(clear(),110)-bass::gain(-6))<0.001,"Output must follow dB gain");
        s=clear(); s.colors[2]=3;
        bass::Engine loudEngine,quietEngine; loudEngine.prepare(48000); quietEngine.prepare(48000);
        Signal loud,quiet; loud.sine(110,48000,0.8f); quiet.sine(110,48000,0.08f);
        loud.run(loudEngine,s); quiet.run(quietEngine,s);
        require(loud.rms()/quiet.rms()<4,"Compression at zero must reduce a 20 dB input dynamic range");
        for(size_t module=3;module<6;++module) {
            s=clear(); s.colors[module]=1;
            require(std::abs(tone(s,110)-tone(clear(),110))>0.0001,"Wet modules must be active at zero");
        }
        bass::Engine whole,chunked; whole.prepare(48000); chunked.prepare(48000);
        Signal a,b; a.sine(110); b=a; a.run(whole,{},48000); b.run(chunked,{},31);
        require(a.l==b.l && a.r==b.r,"DSP must be invariant to host block size");
        bass::Engine e; e.prepare(48000); Signal stereo; stereo.sine(110); stereo.run(e,clear());
        require(stereo.l==stereo.r,"Clear stereo processing must preserve channel equality");
        Signal malformed; malformed.l[0]=NAN; malformed.r[0]=INFINITY; malformed.run(e,{});
        require(std::isfinite(malformed.l[0])&&std::isfinite(malformed.r[0]),"Invalid host samples must not poison DSP");
        std::cout<<"PASS: "<<checks<<" assertions (silence, bypass, EQ, dynamics, effects, sample rates, stereo, block invariance, finite output).\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
