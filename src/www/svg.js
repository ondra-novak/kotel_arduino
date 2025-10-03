//@requires forms.js
/// <reference path="./forms.js"/>


class Svg {
    static create(tag, attributes={}, className = "") {
        const element = document.createElementNS("http://www.w3.org/2000/svg", tag);
        for (const [key, value] of Object.entries(attributes)) {
            element.setAttribute(key, value);
        }
        if (className) {
            element.setAttribute("class", className);
        }
        return element;
    }
};
FormView.controls["svg"] = class extends FormViewControl {    
    clear() {
        this.element().innerHTML="";        
    }
    append(svgel) {
        this.element().appendChild(svgel);
    }
    viewbox(x, y, width, height) {
        this.element().setAttribute("viewBox",`${x} ${y} ${width} ${height}`);
    }
    get() {
        return this;
    }
    size(width,height) {
        this.element().setAttribute("width",`${width}`);
        this.element().setAttribute("height",`${height}`);
    }
}

class SvgPathBuilder {
    #cmds = [];
    #attrs = {};
    #clsn  = "";
    constructor(clsn = "", attrs = {}) {
        this.#attrs = attrs;
        this.#clsn = clsn;
    }
    moveTo(x, y) {
        this.#cmds.push(`M ${x},${y}`);
        return this;
    }
    moveBy(dx, dy) {
        this.#cmds.push(`m ${dx},${dy}`);
        return this;
    }
    lineTo(x, y) {
        this.#cmds.push(`L ${x},${y}`);
        return this;
    }
    lineBy(dx, dy) {
        this.#cmds.push(`l ${dx},${dy}`);
        return this;
    }
    horizontalLineTo(x) {
        this.#cmds.push(`H ${x}`);
        return this;
    }
    horizontalLineBy(dx) {
        this.#cmds.push(`h ${dx}`);
        return this;
    }
    verticalLineTo(y) {
        this.#cmds.push(`V ${y}`);
        return this;
    }
    verticalLineBy(dy) {
        this.#cmds.push(`v ${dy}`);
        return this;
    }
    arcTo(rx, ry, xAxisRotation, largeArcFlag, sweepFlag, x, y) {
        this.#cmds.push(`A ${rx} ${ry} ${xAxisRotation} ${largeArcFlag} ${sweepFlag} ${x} ${y}`);
        return this;
    }
    circle(radius) {
        this.#cmds.push(`m 0,${-radius}`);
        this.#cmds.push(`a ${radius} ${radius} 0 1 1 0,${radius}`);
        this.#cmds.push(`a ${radius} ${radius} 0 1 1 0,${-radius}`);
        return this;
    }
    bezierCurveTo(x1, y1, x2, y2, x, y) {
        this.#cmds.push(`C ${x1} ${y1}, ${x2} ${y2}, ${x} ${y}`);
        return this;
    }
    closePath() {
        this.#cmds.push("Z");
        return this;
    }
    empty() {
        return this.#cmds.length == 0;
    }
    build() {
        const pd = this.#cmds.join(" ");
        const pa = { d: pd, ...this.#attrs};
        return Svg.create("path", pa, this.#clsn);
    }
}

class SvgTextGroupBuilder {
    #t = [];
    #a = {};
    #c ="";
    constructor(c = "", a = {}) {        
        this.#a = a;
        this.#c = c;
    }

    add(x, y, t) {
        this.#t.push({ x, y, t: t});
        return this;
    }

    build() {
        const g = Svg.create("g", this.#a, this.#c);

        this.#t.forEach(itm=>{
            const el = Svg.create("text",{x:itm.x, y:itm.y});
            el.textContent = itm.t;
            g.appendChild(el);
        });
        return g;
    }

    empty() {
        return this.#t.length == 0;
    }
}

