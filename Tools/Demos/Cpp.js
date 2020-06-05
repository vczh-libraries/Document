
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

    tooltipElement.style.left = (offsetX - scrollX) + 'px';
    tooltipElement.style.top = (offsetY - scrollY) + 'px';
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
                idGroup.omitted === null ? '' : `<tr><td colspan="2" class="dropdownData omitted">${idGroup.omitted}</td></tr>`
                }
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
 *   omitted: null | string         // some result is omitted in other groups
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

function jumpToSymbol(overloadResolutions, resolved, ps, primary) {
    closeTooltip();
    if (overloadResolutions.length === resolved.length) {
        if (JSON.stringify(overloadResolutions) === JSON.stringify(resolved)) {
            overloadResolutions = [];
        }
    }

    let omitted = false;
    const filteredResolved = resolved.filter(id => overloadResolutions.indexOf(id) === -1);
    if (filteredResolved.length != resolved.length) {
        omitted = true;
        resolved = filteredResolved;
    }

    const packedArguments = {
        'Overload Resolution': overloadResolutions,
        'Resolved': resolved,
        'Partial Specializations': ps,
        'Primary Declaration': primary,
    };
    const dropdownData = [];

    for (idsKey in packedArguments) {
        const idGroup = { name: idsKey, omitted: null, symbols: [] };
        if (idsKey === 'Resolved' && omitted) {
            idGroup.omitted = '(Duplicated items in "Overload Resolution" are hidden)';
        }
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
        if (idGroup.omitted !== null || idGroup.symbols.length !== 0) {
            dropdownData.push(idGroup);
        }
    }

    if (dropdownData.length === 0) {
        promptTooltipMessage('The target symbol is not defined in the source code.', event.target);
        return;
    }

    if (dropdownData.length === 1 && dropdownData[0].name === 'Resolved') {
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

function toggleSymbolDropdown(symbolExpanding) {
    let symbolElement = symbolExpanding.parentElement.parentElement;
    let containerElement = symbolElement.getElementsByClassName('symbol_dropdown_container')[0];

    if (symbolExpanding.textContent == '-') {
        symbolExpanding.textContent = '+';
        containerElement.classList.remove('expanded');
    } else {
        symbolExpanding.textContent = '-';
        containerElement.classList.add('expanded');
    }

    let status = containerElement.getAttribute('data-status');
    if (status == 'empty' && symbolExpanding.textContent == '-') {
        let uniqueId = containerElement.getAttribute('data-uniqueId');
        let dropdownElement = containerElement.getElementsByClassName('symbol_dropdown')[0];

        containerElement.setAttribute('data-status', 'loading');
        let url = window.location.pathname.substr(0, window.location.pathname.lastIndexOf('/') + 1) + 'SymbolIndexFragments/' + uniqueId + '.html';

        let xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function () {
            if (xhr.readyState == 4) {
                if (xhr.status == 200) {
                    containerElement.setAttribute('data-status', 'loaded');
                    dropdownElement.innerHTML = xhr.responseText;
                } else {
                    containerElement.setAttribute('data-status', 'empty');
                    dropdownElement.textContent = 'Failed to load, please expand again to retry.';
                }
            }
        };
        xhr.open('GET', url);
        xhr.send();
    }
}