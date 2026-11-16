#include "PluginEditor.h"
#if BASS_HAS_ARTWORK
#include <BinaryData.h>
#endif

void BassEditor::MetalLook::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,
    float pos,float,float,const juce::Slider::SliderStyle style,juce::Slider&) {
    if(style==juce::Slider::LinearHorizontal) {
        const float cy=static_cast<float>(y+h/2);
        g.setColour(juce::Colours::black); g.fillRoundedRectangle(static_cast<float>(x),cy-2,static_cast<float>(w),4,2);
        g.setColour(juce::Colour(0xffbabdbf)); g.fillRoundedRectangle(pos-5,cy-6,10,12,2);
        return;
    }
    const float cx=static_cast<float>(x+w/2);
    g.setColour(juce::Colours::black); g.fillRoundedRectangle(cx-3,static_cast<float>(y),6,static_cast<float>(h),3);
    juce::Rectangle<float> cap(cx-15,pos-23,30,46);
    g.setColour(juce::Colours::black.withAlpha(0.5f)); g.fillRoundedRectangle(cap.translated(3,4),3);
    juce::ColourGradient metal(juce::Colour(0xff55595b),cx,pos-23,juce::Colour(0xffeeeeeb),cx,pos+14,false);
    metal.addColour(0.4,juce::Colour(0xffb4b8ba)); metal.addColour(0.5,juce::Colour(0xff777b7e));
    g.setGradientFill(metal); g.fillRoundedRectangle(cap,3);
    g.setColour(juce::Colour(0xffd5d6d4)); g.drawRoundedRectangle(cap,3,1);
    g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawHorizontalLine(static_cast<int>(pos),cx-10,cx+10);
}
BassEditor::BassEditor(BassProcessor& p):AudioProcessorEditor(&p),processor(p) {
    setLookAndFeel(&look);
    const char* names[]={"INPUT SENS","BASS","TREBLE","COMPRESS","SUB","DISTORTION","PITCH","OUTPUT","BALANCE"};
    for(size_t i=0;i<9;++i) {
        auto& s=sliders[i]; addAndMakeVisible(s); s.setName(names[i]);
        s.setSliderStyle(i==8?juce::Slider::LinearHorizontal:juce::Slider::LinearVertical);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,54,18);
        s.setTextValueSuffix(" dB"); s.setDoubleClickReturnValue(true,0);
        s.setTooltip(juce::String(names[i])+": arraste; duplo clique para zero; teclado para ajuste fino.");
        sliderAttachments[i]=std::make_unique<SA>(p.state,BassProcessor::faderIds[i],s);
    }
    for(size_t i=0;i<6;++i) {
        auto* parameter=dynamic_cast<juce::AudioParameterChoice*>(p.state.getParameter(BassProcessor::colorIds[i]));
        auto& c=modes[i]; addAndMakeVisible(c); c.addItemList(parameter->choices,1);
        c.setName(parameter->name); c.setTooltip(parameter->name);
        c.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff252729));
        modeAttachments[i]=std::make_unique<CA>(p.state,BassProcessor::colorIds[i],c);
    }
    for(auto* b:{&reset,&undoButton,&redoButton,&compare,&load,&save}) addAndMakeVisible(b);
    addAndMakeVisible(bypassButton);
    bypassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"bypass",bypassButton);
    reset.onClick=[this]{processor.resetDefaults();};
    undoButton.onClick=[this]{processor.undo.undo();}; redoButton.onClick=[this]{processor.undo.redo();};
    compare.onClick=[this]{auto current=processor.state.copyState(); if(alternate.isValid()) processor.state.replaceState(alternate.createCopy()); alternate=current;};
    alternate=p.state.copyState();
    load.onClick=[this]{choosePreset(false);}; save.onClick=[this]{choosePreset(true);};
#if BASS_HAS_ARTWORK
    artwork=juce::ImageCache::getFromMemory(BinaryData::definitive_png,BinaryData::definitive_pngSize);
#endif
    setSize(900,570); setResizable(false,false); startTimerHz(30);
}
BassEditor::~BassEditor() { stopTimer(); setLookAndFeel(nullptr); }
void BassEditor::resized() {
    reset.setBounds(260,6,48,22); undoButton.setBounds(310,6,48,22); redoButton.setBounds(360,6,48,22);
    compare.setBounds(410,6,48,22); load.setBounds(460,6,48,22); save.setBounds(510,6,48,22);
    bypassButton.setBounds(790,6,90,22);
    sliders[0].setBounds(35,235,62,273); sliders[7].setBounds(810,235,62,273);
    sliders[8].setBounds(100,510,80,42);
    sliders[8].setEnabled(processor.getTotalNumInputChannels()==2);
    for(int i=0;i<6;++i) {
        const int x=210+i*90;
        sliders[static_cast<size_t>(i+1)].setBounds(x,235,62,273);
        modes[static_cast<size_t>(i)].setBounds(x-8,186,80,30);
    }
}
void BassEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff202225));
    if(artwork.isValid()) {
        // Preserve native artwork pixels. Controls remain an independent overlay.
        g.drawImageAt(artwork,0,0);
    } else {
        juce::ColourGradient background(juce::Colour(0xff414448),0,30,juce::Colour(0xff181a1b),0,570,false);
        g.setGradientFill(background); g.fillRoundedRectangle(8,35,884,525,18);
        g.setColour(juce::Colour(0xff111213)); g.fillRoundedRectangle(20,42,860,97,12);
        // Deterministic grain is a temporary panel, independent of the definitive artwork.
        juce::Random r(4107); for(int i=0;i<3800;++i) {
            g.setColour(juce::Colours::white.withAlpha(r.nextFloat()*0.05f));
            g.fillRect(22+r.nextInt(856),44+r.nextInt(92),1,2);
        }
        g.setColour(juce::Colour(0xffb3b6b8)); g.drawRoundedRectangle(8,35,884,525,18,3);
        g.setFont(29.f); g.setColour(juce::Colour(0xffdddddb)); g.drawText("Foundation Bass",38,60,700,50,juce::Justification::centredLeft);
        g.setFont(12.f); g.drawText("AULAMIX  /  BASS PROCESSOR",22,4,238,25,juce::Justification::centredLeft);
    }
    const char* names[]={"INPUT SENS","BASS","TREBLE","COMPRESS","SUB","DISTORTION","PITCH","OUTPUT"};
    g.setFont(12.f); g.setColour(juce::Colour(0xffdadbd8));
    for(int i=0;i<8;++i) {
        auto b=sliders[static_cast<size_t>(i)].getBounds();
        g.drawText(names[i],b.getX()-10,510,b.getWidth()+20,18,juce::Justification::centred);
        for(int j=0;j<3;++j) g.drawText(j==0?"+10":j==1?"0":"-10",b.getX()-15,250+j*103,24,15,juce::Justification::centred);
    }
    for(size_t i=0;i<6;++i) {
        const int mode=modes[i].getSelectedId()-1;
        const juce::Colour colors[]={juce::Colour(0xff999999),juce::Colour(0xffa9d951),juce::Colour(0xff4cadd5),juce::Colour(0xffe05b54)};
        g.setColour(colors[static_cast<size_t>(juce::jlimit(0,3,mode))]);
        g.drawText(modes[i].getText().toUpperCase(),202+static_cast<int>(i)*90,151,80,24,juce::Justification::centred);
    }
    for(int meter=0;meter<2;++meter) {
        const int x=meter==0?119:765;
        g.setColour(juce::Colours::black); g.fillRect(x-4,237,40,255);
        for(int c=0;c<2;++c) {
            const int idx=meter*2+c;
            const float db=juce::Decibels::gainToDecibels(levels[static_cast<size_t>(idx)],-60.f);
            for(int seg=0;seg<26;++seg) {
                const auto color=seg>23?juce::Colours::red:seg>19?juce::Colours::yellow:juce::Colours::greenyellow;
                g.setColour(color.withAlpha(db>=-26+seg?0.95f:0.08f));
                g.fillRect(x+c*16,476-seg*8,12,6);
            }
            g.setColour(processor.clips[static_cast<size_t>(idx)].load()?juce::Colours::red:juce::Colour(0xff402323));
            g.fillRect(x+c*16,241,12,7);
        }
    }
    const float db=juce::Decibels::gainToDecibels(std::max(levels[0],levels[1]),-100.f);
    g.setColour(db < -36?juce::Colour(0xff34342a):db < -18?juce::Colours::limegreen:db < -6?juce::Colours::yellow:juce::Colours::red);
    g.fillRoundedRectangle(55,216,16,10,2);
}
void BassEditor::timerCallback() {
    for(size_t i=0;i<4;++i) levels[i]=std::max(processor.meters[i].exchange(0),levels[i]*0.91f);
    undoButton.setEnabled(processor.undo.canUndo()); redoButton.setEnabled(processor.undo.canRedo()); repaint();
}
void BassEditor::mouseDown(const juce::MouseEvent& e) {
    if(juce::Rectangle<int>(115,237,40,255).contains(e.getPosition()) || juce::Rectangle<int>(761,237,40,255).contains(e.getPosition())) processor.resetClips();
}
void BassEditor::choosePreset(bool saving) {
    chooser=std::make_unique<juce::FileChooser>(saving?"Salvar preset":"Carregar preset",juce::File{},"*.fbpreset");
    const auto browserFlags=juce::FileBrowserComponent::canSelectFiles | (saving?juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting:juce::FileBrowserComponent::openMode);
    chooser->launchAsync(browserFlags,[safe=juce::Component::SafePointer<BassEditor>(this),saving](const juce::FileChooser& fc){
        if(safe==nullptr) return;
        auto file=fc.getResult(); if(file==juce::File{}) return;
        bool ok=false;
        if(saving) {
            juce::MemoryBlock data; safe->processor.getStateInformation(data);
            ok=file.withFileExtension("fbpreset").replaceWithData(data.getData(),data.getSize());
        } else {
            juce::MemoryBlock data;
            if(file.getSize()<1024*1024 && file.loadFileAsData(data)) {
                auto xml=juce::AudioProcessor::getXmlFromBinary(data.getData(),static_cast<int>(data.getSize()));
                if(xml && xml->hasTagName("FoundationBassState")) {safe->processor.setStateInformation(data.getData(),static_cast<int>(data.getSize())); ok=true;}
            }
        }
        if(!ok) juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Preset","Não foi possível salvar ou ler um preset válido.");
    });
}
