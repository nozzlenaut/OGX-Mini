// Tiny test to check the two-button menu using a normal computer.
// No Pico, Bluetooth module, or OLED is needed to test the choices.
#include "../MenuLogic.h"
#include <cassert>
int main() {
    standalone_ui::MenuLogic m;
    m.begin(1);
    assert(m.output()==1);                          // Default: OG Xbox.
    assert(m.select()==standalone_ui::Action::Pair); // Select PAIR.
    m.next();                                        // Highlight OUTPUT.
    m.select();                                      // Enter mode chooser.
    m.next();                                        // Next: Steel Battalion.
    assert(m.output()==2 && m.changed());
    m.select();                                      // Confirm, back to menu.
    m.next();                                        // Highlight WEB APP.
    assert(m.select()==standalone_ui::Action::WebApp);
    m.next();                                        // Highlight SAVE.
    assert(m.select()==standalone_ui::Action::Save);
}
