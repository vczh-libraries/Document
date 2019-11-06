
let lastFocusedElement = undefined;
let referencedSymbols = undefined;
let symbolToFiles = undefined;

function turnOffCurrentSymbol() {
    if (lastFocusedElement !== undefined) {
        lastFocusedElement.classList.remove('focused');
    }
}

function turnOnCurrentSymbol() {
    if (lastFocusedElement !== undefined) {
        lastFocusedElement.classList.add('focused');
    }
}

function turnOnSymbol(id) {
    if (id === undefined) {
        id = decodeURIComponent(window.location.hash.substring(1));
    }
    if (id === '') {
        return;
    }

    turnOffCurrentSymbol();
    lastFocusedElement = document.getElementById(id);
    turnOnCurrentSymbol();
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
    <div class="tooltip" onmouseleave="closeTooltip()" onclick="event.stopPropagation()">
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
    const htmlCode = `<div class="tooltipContent message">${message}</div>`;
    const tooltipContent = new DOMParser().parseFromString(htmlCode, 'text/html').getElementsByClassName('tooltipContent')[0];
    promptTooltip(tooltipContent, underElement);
}

function promptTooltipDropdownData(dropdownData, underElement) {
    const htmlCode = `
    <div class="tooltipContent">
    <table>
    ${
        dropdownData.map(function (idGroup) {
            return `
            <tr><td colspan="2" class="dropdownData idGroup">${idGroup.name}</td></tr>
            ${
                idGroup.symbols.map(function (symbol) {
                    return `
                    <tr><td colspan="2" class="dropdownData symbol">${symbol.displayNameInHtml}</td></tr>
                    ${
                        symbol.decls.map(function (decl) {
                            return `
                            <tr>
                                <td class="dropdownData label"><span>${decl.label}</span></td>
                                <td class="dropdownData link">
                                    <a onclick="${decl.file === null ? `closeTooltip(); jumpToSymbolInThisPage('${decl.elementId}');` : `closeTooltip(); jumpToSymbolInOtherPage('${decl.elementId}', '${decl.file.htmlFileName}');`}">
                                        ${decl.file === null ? 'Highlight it' : decl.file.displayName}
                                    </a>
                                </td>
                            </tr>
                            `;
                        }).join('')
                    }
                    `;
                }).join('')
            }
            `;
        }).join('')
    }
    </table>
    </div>`;
    const tooltipContent = new DOMParser().parseFromString(htmlCode, 'text/html').getElementsByClassName('tooltipContent')[0];
    promptTooltip(tooltipContent, underElement);
}

/*
 * dropdownData: {
 *   name: string,                  // group name
 *   symbols: {
 *     displayNameInHtml: string,   // display name
 *     decls: {
 *       label: string,             // the label of this declaration
 *       file: {
 *          displayName: string,    // the display name of the file
 *          htmlFileName: string    // the html file name of the file without ".html"
 *       },
 *       elementId: string          // the element id of this declaration
 *     }[]
 *   }[]
 * }[]
 *
 * referencedSymbols: {
 *   [key: string]: {
 *     displayNameInHtml: string,
 *     impls: string[],
 *     decls: string[]
 *   }
 * }
 *
 * symbolToFiles: {
 *   [key: string]: null | {
 *     htmlFileName: string,
 *     displayName: string
 *   }
 * }
 */

function jumpToSymbol(overloadResolutions, resolved) {
    closeTooltip();
    if (overloadResolutions.length === 1 && resolved.length === 1 && overloadResolutions[0] === resolved[0]) {
        overloadResolutions = [];
    }
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
                const symbol = { displayNameInHtml: referencedSymbol.displayNameInHtml, decls: [] };

                for (i = 0; i < referencedSymbol.impls.length; i++) {
                    const elementId = referencedSymbol.impls[i];
                    const label = 'impl[' + i + ']';
                    const file = symbolToFiles[elementId];
                    if (file !== undefined) {
                        symbol.decls.push({ label, file, elementId });
                    }
                }

                for (i = 0; i < referencedSymbol.decls.length; i++) {
                    const elementId = referencedSymbol.decls[i];
                    const label = 'decl[' + i + ']';
                    const file = symbolToFiles[elementId];
                    if (file !== undefined) {
                        symbol.decls.push({ label, file, elementId });
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
        promptTooltipMessage('The target symbol is not defined in the source code.', event.target);
        return;
    }

    if (dropdownData.length === 1) {
        const idGroup = dropdownData[0];
        if (idGroup.symbols.length === 1) {
            const symbol = idGroup.symbols[0];
            if (symbol.decls.length === 1) {
                const decl = symbol.decls[0];
                if (decl.file === null) {
                    jumpToSymbolInThisPage(decl.elementId);
                }
                else {
                    jumpToSymbolInOtherPage(decl.elementId, decl.file.htmlFileName);
                }
                return;
            }
        }
    }

    promptTooltipDropdownData(dropdownData, event.target);
}