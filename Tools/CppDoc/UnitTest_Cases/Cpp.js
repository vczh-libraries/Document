
let lastFocusedElement = undefined;
let referencedSymbols = undefined;
let symbolToFiles = undefined;

function jumpToSymbol(overloadResolutions, resolved) {
    return;
    if (lastFocusedElement !== undefined) {
        lastFocusedElement.classList.remove('focused');
    }
    lastFocusedElement = document.getElementById('symbol$' + symbolName[0]);
    if (lastFocusedElement !== undefined) {
        lastFocusedElement.classList.add('focused');
        window.location.href = '#';
        window.location.href = '#' + lastFocusedElement.id;
    }
}