// Copyright (C) 2016 xaizek <xaizek@openmailbox.org>
//
// This application is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This application is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this application.  If not, see <http://www.gnu.org/licenses/>.

// Compilation: g++ list-picker.cpp -lncurses++w -lformw -lmenuw -lpanelw -lncursesw -o list-picker

#include <cursesapp.h>
#include <cursesm.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <clocale>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <stdexcept>

class MenuItems
{
protected:
    std::vector<NCursesMenuItem *> itemList;

    ~MenuItems()
    {
        typedef std::vector<NCursesMenuItem *>::const_iterator it;
        for (it i = itemList.begin(); i != itemList.end(); ++i) {
            delete *i;
        }
    }
};

// Using Base-from-Member idiom to destroy items after menu.
class Menu : private MenuItems, public NCursesMenu
{
public:
    explicit Menu(const std::vector<std::string> &in)
        : NCursesMenu(lines(), cols()), accepted(false), items(in)
    {
        set_format(lines() - 1, 1); // Make menu large.
        set_mark("*");              // Use * as a marker.
        options_off(O_ONEVALUE);    // Allow multi-selection.

        // Compose list of menu items.
        itemList.reserve(items.size() + 1U);
        typedef std::vector<std::string>::const_iterator it;
        for (it i = items.begin(); i != items.end(); ++i) {
            itemList.push_back(new NCursesMenuItem(i->c_str()));
        }
        itemList.push_back(new NCursesMenuItem());

        // Initialize NCursesMenu.
        InitMenu(&itemList[0], false, false);
    }

    std::vector<std::string> getSelection()
    {
        std::vector<std::string> selection;

        // Empty selection if choice wasn't confirmed.
        if (!accepted) {
            return selection;
        }

        // Use selected items.
        typedef std::vector<NCursesMenuItem *>::const_iterator it;
        for (it i = itemList.begin(); i != itemList.end(); ++i) {
            if ((*i)->value()) {
                selection.push_back((*i)->name());
            }
        }

        // Or just the current item.
        if (selection.empty()) {
            selection.push_back(current_item()->name());
        }

        return selection;
    }

    virtual int virtualize(int c)
    {
        switch (c) {
            // Down.
            case 'j':  return REQ_DOWN_ITEM;
            // Up.
            case 'k':  return REQ_UP_ITEM;
            // Toggle item and don't move cursor.
            case ' ':  return REQ_TOGGLE_ITEM;
            case 't':  return REQ_TOGGLE_ITEM;
            // Toggle item and move cursor down.
            case 'T':  current_item()->set_value(!current_item()->value());
                       return REQ_DOWN_ITEM;

            // Quit accepting the selection.
            case '\n': accepted = true;
                       return MAX_COMMAND + 1;
            // Cancel.
            case 'q':  return MAX_COMMAND + 1;

            // Pass the rest of the keys to predefined handler.
            default: return NCursesMenu::virtualize(c);
        }
    }

private:
    bool accepted;
    const std::vector<std::string> items;
};

/* Opens controlling terminal doing its best at guessing its name.  This might
 * not work if all the standard streams are redirected, in which case /dev/tty
 * is used, which can't be passed around between processes, so some applications
 * won't work in this (quite rare) case.  Returns opened file descriptor or -1
 * on error. */
static int open_tty(void)
{
    int fd = -1;

    /* XXX: we might be better off duplicating descriptor if possible. */

    if (isatty(STDIN_FILENO)) {
        fd = open(ttyname(STDIN_FILENO), O_RDWR);
    }
    if (fd == -1 && isatty(STDOUT_FILENO)) {
        fd = open(ttyname(STDOUT_FILENO), O_RDWR);
    }
    if (fd == -1 && isatty(STDERR_FILENO)) {
        fd = open(ttyname(STDERR_FILENO), O_RDWR);
    }

    if (fd == -1) {
        fd = open("/dev/tty", O_RDWR);
    }

    return fd;
}

// Reopens real terminal and binds it to stdin.
static void reopen_term_stdin(void)
{
    if (close(STDIN_FILENO)) {
        throw std::runtime_error("Failed to close original input stream.");
    }

    const int ttyfd = open_tty();
    if (ttyfd != STDIN_FILENO) {
        if (ttyfd != -1) {
            close(ttyfd);
        }

        throw std::runtime_error("Failed to open terminal for input.\n");
    }
}

class App : public NCursesApplication
{
    std::vector<std::string> args;

protected:
    virtual int titlesize() const
    {
        return 1;
    }

    virtual void title()
    {
        titleWindow->bkgd(screen_titles());
        titleWindow->erase();
        titleWindow->addstr(0, 0, "space:Toggle  enter:Accept  q:Cancel");
        titleWindow->noutrefresh();
    }

public:
    App() : NCursesApplication(false)
    {
    }

    virtual void handleArgs(int argc, char *argv[])
    {
        args.assign(argv + 1, argv + argc);
    }

    virtual int run()
    {
        std::vector<std::string> in;

        // Either use list provided as arguments or read from standard input.
        if (!args.empty()) {
            in = args;
        } else {
            typedef std::istream_iterator<std::string> it;
            in.assign(it(std::cin), it());
            reopen_term_stdin();

            if (in.empty()) {
                // Disable curses to be able to print onto standard output.
                endwin();
                std::cerr << "No items to work with" << std::endl;
                return EXIT_FAILURE;
            }
        }

        Menu m(in);
        static_cast<void>(m());

        // Disable curses to be able to print onto standard output.
        endwin();

        std::vector<std::string> selection = m.getSelection();
        std::copy(selection.begin(), selection.end(),
                  std::ostream_iterator<std::string>(std::cout, "\n"));

        return selection.empty() ? EXIT_FAILURE : EXIT_SUCCESS;
    }
};

int main(int argc, char *argv[])
{
    std::setlocale(LC_ALL, "");

    int res;

    try {
        App app;
        app.handleArgs(argc, argv);
        res = app();
        endwin();
    } catch (const NCursesException *e) {
        endwin();
        std::cerr << e->message << std::endl;
        res = e->errorno;
    } catch (const NCursesException &e) {
        endwin();
        std::cerr << e.message << std::endl;
        res = e.errorno;
    } catch (const std::exception &e) {
        endwin();
        std::cerr << "Exception: " << e.what() << std::endl;
        res = EXIT_FAILURE;
    }
    return res;
}
