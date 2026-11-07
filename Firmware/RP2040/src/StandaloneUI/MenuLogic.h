#pragma once
#include <cstdint>

namespace standalone_ui {
// Pure menu logic: no Pico hardware required, making it easy to test.
// The numbers match DeviceDriverTypes.h in the original OGX-Mini source.
struct OutputMode { uint8_t id; const char* label; };
static constexpr OutputMode OUTPUTS[]={
    {1,"XBOX OG"}, {2,"STEEL BTN"}, {4,"XINPUT"}, {5,"PS3"},
    {6,"DINPUT"}, {7,"PS CLASS"}, {8,"SWITCH"}
};
static constexpr unsigned OUTPUT_COUNT=sizeof(OUTPUTS)/sizeof(OUTPUTS[0]);
enum class Page : uint8_t { Main, Output };
enum class Action : uint8_t { None, Pair, WebApp, Save };

class MenuLogic {
public:
    void begin(uint8_t previously_saved) {
        saved_=previously_saved;
        choice_=0; output_index_=0; page_=Page::Main;
        for(unsigned i=0;i<OUTPUT_COUNT;++i)
            if(OUTPUTS[i].id==saved_) output_index_=i;
    }
    Page page() const { return page_; }
    unsigned choice() const { return choice_; }
    uint8_t output() const { return OUTPUTS[output_index_].id; }
    const char* output_name() const { return OUTPUTS[output_index_].label; }
    bool changed() const { return output()!=saved_; }
    void next() {
        // First button navigates menu OR cycles output choices.
        if(page_==Page::Output) output_index_=(output_index_+1)%OUTPUT_COUNT;
        else choice_=(choice_+1)%4;
    }
    Action select() {
        // Second button selects. In output selection, it means "go back".
        if(page_==Page::Output){page_=Page::Main;return Action::None;}
        if(choice_==0)return Action::Pair;
        if(choice_==1){page_=Page::Output;return Action::None;}
        if(choice_==2)return Action::WebApp;
        return Action::Save;
    }
private:
    uint8_t saved_=1;
    unsigned choice_=0, output_index_=0;
    Page page_=Page::Main;
};
}
