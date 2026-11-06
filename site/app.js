import { showVlcHotkeys } from "./remote/vlc-hotkeys.js";
import { mountTextFitting } from "./remote/text-fit.js";
showVlcHotkeys(document.getElementById("vlc-hotkeys"));
await mountTextFitting();
