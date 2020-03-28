"use strict";

class GameContextMenu {
    static _context_menu = null;

    static init()
    {
        document.addEventListener("readystatechange", event => {
            if (event.target.readyState === "complete") {
                GameContextMenu._context_menu = new GameContextMenu(".menu");

                document.addEventListener('contextmenu', (e) => {
                    GameContextMenu._context_menu.handleContextMenu(e);
                }, false);
            }
        });
    }

    static onClick(event)
    {
        GameContextMenu._context_menu.handleClick(event);
    }

    constructor(id)
    {
        this._menu = document.querySelector(id);
    }

    show(x, y)
    {
        if (undefined === this._menu) { return; }

        this._menu.style.left = x + 'px';
        this._menu.style.top  = y + 'px';
        this._menu.classList.add('show-menu');
    }

    hide()
    {
        this._menu.classList.remove('show-menu');
    }

    handleContextMenu(event)
    {
        event.preventDefault();

        this.show(event.pageX, event.pageY);
        document.addEventListener('click', GameContextMenu.onClick, false);
    }

    handleClick(event)
    {
        //event.preventDefault();

        this.hide();
        document.removeEventListener('click', GameContextMenu.onClick);
    }

};
