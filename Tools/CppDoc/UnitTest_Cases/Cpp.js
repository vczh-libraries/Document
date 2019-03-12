
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

function jumpToSymbolInThisPage(id) {
    turnOnSymbol(id);
    if (lastFocusedElement !== undefined) {
        window.location.href = '#';
        window.location.href = '#' + symbolId;
    }
}

function jumpToSymbolInOtherPage(id, file) {
    alert('Not implemented.');
}

function jumpToSymbol(overloadResolutions, resolved) {
    for (ids of [overloadResolutions, resolved]) {
        for (id of ids) {
            let symbol = referencedSymbols[id];
            if (symbol !== undefined) {
                if (symbol.definition === true) {
                    let symbolId = 'Decl$' + id;
                    let file = symbolToFiles[symbolId];
                    if (file === '') {
                        jumpToSymbolInThisPage(symbolId);
                        return;
                    } else if (file !== undefined) {
                        jumpToSymbolInOtherPage(symbolId, file);
                        return;
                    }
                }
                for (i = 0; i < symbol.declarations; i++) {
                    let symbolId = 'Forward[' + i + ']$' + id;
                    let file = symbolToFiles[symbolId];
                    if (file === '') {
                        jumpToSymbolInThisPage(symbolId);
                        return;
                    } else if (file !== undefined) {
                        jumpToSymbolInOtherPage(symbolId, file);
                        return;
                    }
                }
            }
        }
    }
}