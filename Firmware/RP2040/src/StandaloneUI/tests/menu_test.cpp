// Menu unit tests run on a normal computer. No Pico or OLED needed.
// They catch accidental changes to the two-button navigation before flashing.
#include "../MenuLogic.h"
#include <cassert>
int main() {
    using namespace standalone_ui;
    MenuLogic m;

    // First boot reads a saved Xbox OG output and highlights PAIR.
    m.begin(1);
    assert(m.output()==1 && !m.changed());
    assert(m.select()==Action::Pair);

    // A goes to OUTPUT; B opens the output picker.
    m.next();
    assert(m.choice()==1);
    assert(m.select()==Action::None && m.page()==Page::Output);

    // A steps through seven supported outputs; the list must wrap around.
    const uint8_t expected[]={2,4,5,6,7,8,1};
    for(uint8_t output: expected) {
        m.next();
        assert(m.output()==output);
    }
    assert(!m.changed()); // We are back where we began.

    // Select XInput, confirm, and ensure the menu reports pending changes.
    m.next(); // Steel Battalion
    m.next(); // XInput
    assert(m.output()==4 && m.changed());
    assert(m.select()==Action::None && m.page()==Page::Main);

    // Advance to WebApp, then Save. These must remain separate actions.
    m.next();
    assert(m.select()==Action::WebApp);
    m.next();
    assert(m.select()==Action::Save);

    // Restart / returning from WebApp reloads whatever was saved.
    m.begin(8);
    assert(m.output()==8 && !m.changed()); // Switch.
    assert(m.select()==Action::Pair);      // First selection still PAIR.
}
