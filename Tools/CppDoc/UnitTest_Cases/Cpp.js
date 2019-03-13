
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

/*
 * dropdownData: {
 *   name: string;          // group name
 *   symbols: {
 *     name: string;        // symbol name
 *     decls: {
 *       file: string;      // file that contain this declaration
 *       elementId;          // the element id of this declaration
 *     }[]
 *   }[]
 * }[]
 */

function jumpToSymbol(overloadResolutions, resolved) {
    const packedArguments = {
        'Overload Resolution': overloadResolutions,
        'Resolved': resolved
    };
    const dropdownData = [];

    for (idsKey in packedArguments) {
        const idGroup = { name: idsKey, symbols: [] };
        for (uniqueId of packedArguments[idsKey]) {
            const referencedSymbol = referencedSymbols[uniqueId];
            if (referencedSymbol !== undefined) {
                const symbol = { name: referencedSymbol.name, decls: [] };

                if (referencedSymbol.definition === true) {
                    const elementId = 'Decl$' + uniqueId;
                    const file = symbolToFiles[elementId];
                    if (file !== undefined) {
                        symbol.decls.push({ file, elementId });
                    }
                }

                for (i = 0; i < referencedSymbol.declarations; i++) {
                    const elementId = 'Forward[' + i + ']$' + uniqueId;
                    const file = symbolToFiles[elementId];
                    if (file !== undefined) {
                        symbol.decls.push({ file, elementId });
                    }
                }

                if (symbol.decls.length !== 0) {
                    idGroup.symbols.push(symbol);
                }
            }
        }
        if (idGroup.symbols.length !== 0) {
            dropdownData.push(idGroup);
        }
    }

    if (dropdownData.length === 0) {
        alert('The target symbol is not defined in the source code.');
        return;
    }

    if (dropdownData.length === 1) {
        const idGroup = dropdownData[0];
        if (idGroup.symbols.length === 1) {
            const symbol = idGroup.symbols[0];
            if (symbol.decls.length === 1) {
                const decl = symbol.decls[0];
                if (decl.file === '') {
                    jumpToSymbolInThisPage(decl.elementId);
                }
                else {
                    jumpToSymbolInOtherPage(decl.elementId, decl.file);
                }
                return;
            }
        }
    }

    alert('Multiple symbols jumping is not implemented yet.');
}