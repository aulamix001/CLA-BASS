#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout BassProcessor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    const char* names[]={"Input sensitivity","Bass","Treble","Compress","Sub","Distortion","Pitch","Output","Balance"};
    for(size_t i=0;i<9;++i) {
        const float max=i==8?6.f:10.f;
        p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{faderIds[i],1},names[i],
            juce::NormalisableRange<float>{-max,max,0.1f},0.f));
    }
    const juce::StringArray choices[]={
        {"Bypass","Sub","Lower","Upper"},{"Bypass","Honk","Bark","Cut"},
        {"Bypass","Push","Spank","Wall"},{"Off","On"},
        {"Mute","Growl","Roar","Rip"},{"Mute","Chorus","Wide","Spreader"}};
    for(size_t i=0;i<6;++i)
        p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{colorIds[i],1},
            juce::String(names[i+1])+" Color",choices[i],1));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass",1},"Bypass",false));
    return p;
}
BassProcessor::BassProcessor()
  : AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
      .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
    state(*this,&undo,"FoundationBassState",layout()) {
    for(size_t i=0;i<9;++i) faders[i]=state.getRawParameterValue(faderIds[i]);
    for(size_t i=0;i<6;++i) colors[i]=state.getRawParameterValue(colorIds[i]);
    bypass=state.getRawParameterValue("bypass");
}
bool BassProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo()
        && (l.getMainInputChannelSet()==juce::AudioChannelSet::mono()
            || l.getMainInputChannelSet()==juce::AudioChannelSet::stereo());
}
void BassProcessor::prepareToPlay(double sr,int) { engine.prepare(sr); resetClips(); }
void BassProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) {
    juce::ScopedNoDenormals guard;
    if(buffer.getNumChannels()<2) return;
    if(getTotalNumInputChannels()==1) buffer.copyFrom(1,0,buffer,0,0,buffer.getNumSamples());
    bass::Settings s;
    for(size_t i=0;i<9;++i) s.faders[i]=faders[i]->load();
    for(size_t i=0;i<6;++i) s.colors[i]=static_cast<int>(colors[i]->load());
    s.bypass=bypass->load()>0.5f;
    engine.process(buffer.getWritePointer(0),buffer.getWritePointer(1),buffer.getNumSamples(),s);
    for(size_t i=0;i<4;++i) {
        const float peak=i<2?engine.inputPeak[i]:engine.outputPeak[i-2];
        // Peak latch: the GUI consumes peaks at its own refresh rate.
        float previous=meters[i].load();
        while(previous<peak && !meters[i].compare_exchange_weak(previous,peak)) {}
        if(peak>1) clips[i].store(true);
    }
}
juce::AudioProcessorParameter* BassProcessor::getBypassParameter() const { return state.getParameter("bypass"); }
void BassProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) {
    if(buffer.getNumChannels()>=2 && getTotalNumInputChannels()==1)
        buffer.copyFrom(1,0,buffer,0,0,buffer.getNumSamples());
}
void BassProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if(auto xml=state.copyState().createXml()) copyXmlToBinary(*xml,dest);
}
void BassProcessor::setStateInformation(const void* data,int size) {
    if(auto xml=getXmlFromBinary(data,size))
        if(xml->hasTagName(state.state.getType())) state.replaceState(juce::ValueTree::fromXml(*xml));
}
void BassProcessor::resetDefaults() {
    undo.beginNewTransaction("Reset");
    for(auto* p:getParameters()) {
        p->beginChangeGesture(); p->setValueNotifyingHost(p->getDefaultValue()); p->endChangeGesture();
    }
}
juce::AudioProcessorEditor* BassProcessor::createEditor() { return new BassEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BassProcessor; }
