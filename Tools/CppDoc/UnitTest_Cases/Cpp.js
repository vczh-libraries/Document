
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
    <div class="tooltip" onclick="event.stopPropagation()">
    <div>&nbsp;</div>
    <div class="tooltipHeader">
        <div class="tooltipHeaderBorder"></div>
        <div class="tooltipHeaderFill"></div>
    </div>
    <div class="tooltipContainer" onmouseleave="closeTooltip()"></div>
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
            <tr><td colspan="3" class="dropdownData idGroup">${idGroup.name}</td></tr>
            ${
                idGroup.symbols.map(function (symbol) {
                    return `
                    <tr><td rowspan="${symbol.decls.length + 1}" class="dropdownData symbol">${symbol.name}</td></tr>
                    ${
                        symbol.decls.map(function (decl) {
                            return `
                            <tr>
                                <td></td>
                                <td><span class="dropdownData label">${decl.label}</span></td>
                                <td>
                                    <a class="dropdownData link" onclick="${decl.file === '' ? `closeTooltip(); jumpToSymbolInThisPage('${decl.elementId}');` : `closeTooltip(); jumpToSymbolInOtherPage('${decl.elementId}', '${decl.file}');`}">
                                        ${decl.file === '' ? 'Jump To' : decl.file}
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
 *   name: string;          // group name
 *   symbols: {
 *     name: string;        // symbol name
 *     decls: {
 *       label: string;     // the label of this declaration
 *       file: string;      // file that contain this declaration
 *       elementId;         // the element id of this declaration
 *     }[]
 *   }[]
 * }[]
 */

function jumpToSymbol(overloadResolutions, resolved) {
    closeTooltip();
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
                    const label = 'declaration';
                    const file = symbolToFiles[elementId];
                    if (file !== undefined) {
                        symbol.decls.push({ label, file, elementId });
                    }
                }

                for (i = 0; i < referencedSymbol.declarations; i++) {
                    const elementId = 'Forward[' + i + ']$' + uniqueId;
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

    promptTooltipDropdownData(dropdownData, event.target);
}