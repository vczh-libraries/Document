
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

function closeTooltip() {
    const tooltipElement = document.getElementsByClassName('tooltip')[0];
    if (tooltipElement !== undefined) {
        tooltipElement.remove();
    }
}

function promptTooltip(contentElement, underElement) {
    closeTooltip();
    const tooltipElement = new DOMParser().parseFromString(`
<div class="tooltip" onclick="event.stopPropagation();">
<div>&nbsp;</div>
<div class="tooltipHeader">
    <div class="tooltipHeaderBorder"></div>
    <div class="tooltipHeaderFill"></div>
</div>
<div class="tooltipContainer"></div>
</div>
`, 'text/html').getElementsByClassName('tooltip')[0];
    tooltipElement.getElementsByClassName('tooltipContainer')[0].appendChild(contentElement);

    var elementRect = underElement.getBoundingClientRect();
    var bodyRect = document.body.parentElement.getBoundingClientRect();
    var offsetX = elementRect.left - bodyRect.left;
    var offsetY = elementRect.top - bodyRect.top;
    var scrollX = document.body.scrollLeft + document.body.parentElement.scrollLeft;
    var scrollY = document.body.scrollTop + document.body.parentElement.scrollTop;

    tooltipElement.style.left = (offsetX - scrollX) + "px";
    tooltipElement.style.top = (offsetY - scrollY) + "px";
    underElement.appendChild(tooltipElement);
}

function promptTooltipMessage(message, underElement) {
    const htmlCode = `<div class="tooltipContent">${message}</div>`;
    const tooltipContent = new DOMParser().parseFromString(htmlCode, 'text/html').getElementsByClassName('tooltipContent')[0];
    promptTooltip(tooltipContent, underElement);
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

    promptTooltipMessage('Multiple symbols jumping is not implemented yet.', event.target);
}