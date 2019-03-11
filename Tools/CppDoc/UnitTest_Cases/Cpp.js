
let lastFocusedElement = undefined;

function jumpToSymbol(symbolName) {
    if (lastFocusedElement !== undefined) {
        lastFocusedElement.classList.remove('focused');
    }
    lastFocusedElement = document.getElementById('symbol$' + symbolName[0]);
    if (lastFocusedElement !== undefined) {
        lastFocusedElement.classList.add('focused');
        window.location.href = '#' + lastFocusedElement.id;
    }
}