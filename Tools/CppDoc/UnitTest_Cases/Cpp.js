
let lastFocusedElement = undefined;
let referencedSymbols = undefined;
let symbolToFiles = undefined;

function turnOnSymbol(id) {
    if (id === undefined) {
        id = decodeURIComponent(window.location.hash.substring(1));
    }
    if (id === '') {
        return;
    }

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
        window.location.hash = id;
    }
}

function jumpToSymbolInOtherPage(id, file) {
    window.location.href = './' + file + '.html#' + id;
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