
let lastFocusedElement = undefined;
let referencedSymbols = undefined;
let symbolToFiles = undefined;

function turnOnSymbol(id) {
    if (lastFocusedElement !== undefined) {
        lastFocusedElement.classList.remove('focused');
    }
    lastFocusedElement = document.getElementById(id);
    if (lastFocusedElement !== undefined) {
        lastFocusedElement.classList.add('focused');
    }
}

function jumpToSymbol(overloadResolutions, resolved) {
    for (ids of [overloadResolutions, resolved]) {
        for (id of ids) {

        }
    }
}