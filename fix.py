import json

replies = [
    {
        "comment_id": "4557987223",
        "reply": "Thank you for the update! If `Neverball._module` is failing with that specific XHR fetch error, that means the `fetch` polyfill flag (`ENABLE_FETCH=1`) combined with the way JS interacts with Emscripten's asynchronous module loader might be getting tripped up due to the file protocol, CORS, or a delayed XHR sync.\n\nActually, since the main engine initialization natively mounts the `fs`, the most robust approach is simply using Emscripten's provided virtual filesystem API directly from JavaScript. Instead of using `ccall` to run an exported C function, we can just dump the `.map` file into the Emscripten memory filesystem via JavaScript `Neverball._module.FS.writeFile('/test.map', mapText);`, and then trigger the compilation via a simpler function. Wait, actually I see that we've already enabled `EXPORTED_RUNTIME_METHODS=ccall`, but Emscripten fetch API modifies `XMLHttpRequest` responses. \n\nI believe the error is happening inside `Neverball.run()` or related initialization where `fetch` is trying to access `available-packages.txt` synchronously in a way `gl4es` or Emscripten isn't happy with right after clicking Play. Since this only happens when you click 'Play Test Map' *before* the engine is fully booted up, I'll update the `editor.js` so it doesn't trigger the game start logic if it isn't ready. The `Editor.playMap` will just verify the engine is fully running before executing."
    }
]

with open('replies.json', 'w') as f:
    json.dump(replies, f)
