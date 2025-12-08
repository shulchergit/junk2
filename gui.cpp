#include <stdlib.h>
#include <cmath>
#include <vector>
//#include <set>
#include <iostream>
#include <sstream>
#include <ncurses.h>
#include <string>
#include <algorithm>
#include <ctime>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <signal.h>
#include <chrono>
//#include <sys/types.h>

using namespace std::chrono;
using namespace std;

#include "utility.h"
#include "plctags.h"
#include "list_tags.h"


#define STATE_FILENAME ".plctagt.last"
#define TAG_HISTORY_FILE ".plctagt.taghistory"
const int TAG_HISTORY_MAX = 10;


#define HELP_TEXT \
"Prev Section: CTRL-u\n" \
    "Next Section: CTRL-n\n" \
    "Prev Field: SHIFT-TAB, LEFT\n" \
    "Next Field: TAB, RIGHT\n" \
    "Prev Page Section 3: PGUP\n" \
    "Next Page Section 3: PGDN\n" \
    "Clear Current Field: F5\n" \
    "Clear All Section 3 Fields: F6\n" \
    "Additional Options: F1\n" \
    "Quit: ESC\n" \
    ""

    // ---------- Field & Section ----------
    struct Field {
    string label, value;
    int x, y, width;
    bool editable;
    vector<string> options;
};

struct Section {
    int top, bottom;
    vector<Field> fields;
    int last_active;
};

// Only save Section 1 & 2 editable fields
void saveState(const Section &sec1, const Section &sec2) {
    ofstream out(STATE_FILENAME);
    if(!out) return;

    for(const auto &f : sec1.fields)
        if(f.editable)
            out << f.label << "=" << f.value << "\n";

    for(const auto &f : sec2.fields)
        if(f.editable)
            out << f.label << "=" << f.value << "\n";
}

// Load state into Section 1 & 2 if file exists
void loadState(Section &sec1, Section &sec2) {
    ifstream in(STATE_FILENAME);
    if(!in) return;

    string line;
    while(getline(in, line)) {
        auto pos = line.find('=');
        if(pos == string::npos) continue;
        string label = line.substr(0, pos);     // keep colon!
        string value = line.substr(pos + 1);

        auto apply = [&](Section &sec){
            for(auto &f : sec.fields) {
                if(f.editable && f.label == label)
                    f.value = value;
            }
        };

        apply(sec1);
        apply(sec2);
    }
}


// ---------- NCurses UI ----------
class NCursesUI {
public:
    NCursesUI(vector<Section*> sections): sections(sections) {
        initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(1);
        getmaxyx(stdscr, rows, cols);
        if(cols < 80) cols = 80;
        act_sec = 0;
        act_field = sections[0]->last_active;
        cursor_pos = sections[0]->fields[act_field].value.size();
        scroll = 0;

        updateSection3FromCount();

        loadTagHistory();
    }

    ~NCursesUI() { endwin(); }

    int popupSelect(const std::vector<std::string> &options,
                    const std::string &title = "Select an Option")
    {
        if (options.empty()) return -1;

        curs_set(0); // HIDE CURSOR

        int maxLen = title.size();
        for (const auto &s : options)
            maxLen = std::max<int>(maxLen, s.size());

        int win_w = maxLen + 6;
        int win_h = options.size() + 4;

        int starty = (LINES - win_h) / 2;
        int startx = (COLS - win_w) / 2;

        WINDOW *win = newwin(win_h, win_w, starty, startx);
        keypad(win, TRUE);
        box(win, 0, 0);

        int highlight = 0;
        int choice = -1;
        int ch;

        while (true) {
            mvwprintw(win, 1, (win_w - title.size()) / 2, "%s", title.c_str());

            for (size_t i = 0; i < options.size(); ++i) {
                if (i == (size_t)highlight) wattron(win, A_REVERSE);
                mvwprintw(win, i + 2, 2, "%s", options[i].c_str());
                wattroff(win, A_REVERSE);
            }

            wmove(win, 1, 1);  // <-- prevents cursor from appearing on last item
            wrefresh(win);

            ch = wgetch(win);

            switch (ch) {
            case KEY_UP:
                highlight = (highlight == 0 ? options.size() - 1 : highlight - 1);
                break;
            case KEY_DOWN:
                highlight = (highlight + 1) % options.size();
                break;
            case '\n':
                choice = highlight;
                goto exit_popup;
            case 27:
                choice = -1;
                goto exit_popup;
            }
        }

    exit_popup:
        delwin(win);
        touchwin(stdscr);
        refresh();

        curs_set(1); // RESTORE CURSOR
        return choice;
    }

    void updateUI() {
        clear();
        drawBorders();
        drawSections12Fields();
        drawSection3Fields();
        setCursor();
        refresh();
    }

    void run() {
        std::string last_type = sections[1]->fields[0].value; // track Section 2 type

        bool running = true;
        while(running) {
            updateUI();

            int ch = getch();
            Section* sec = sections[act_sec];
            Field &f = sec->fields[act_field];

            if(!f.options.empty() && ch == 10) { // Enter on field with options
                popupSelectOptions(f);
                continue;
            }

            switch(ch) {
            case 27: running=false; break;
            case KEY_UP:
            case KEY_BTAB: moveFieldUp(); break;
            case KEY_DOWN:
            case '\t': moveFieldDown(); break;
            case 21: moveSectionPrev(); break;
            case 4: moveSectionNext(); break;
            case KEY_PPAGE:
                pageUpSection3();
                break;
            case KEY_NPAGE:
                pageDownSection3();
                break;
            case KEY_F(1):
                popupMessage(HELP_TEXT);
                break;
            case KEY_F(2):
            {
                int r = popupSelect({"LIST TAGS", "TIME PROFILES", }, "");

                int i;
                switch(r) {
                case 0:
                    displayTags();
                    break;
                case 1:
                    r = popupSelect({"TEST ALL", "BIT", "SINT", "INT", "DINT", "LINT", "REAL", "STRING"}, "Stress Tests");
                    switch(r) {
                    case 0:
                        profileTags();
                        break;
                    case 1:
                        profileTags("BIT");
                        break;
                    case 2:
                        profileTags("SINT");
                        break;
                    case 3:
                        profileTags("INT");
                        break;
                    case 4:
                        profileTags("DINT");
                        break;
                    case 5:
                        profileTags("LINT");
                        break;
                    case 6:
                        profileTags("REAL");
                        break;
                    case 7:
                        profileTags("STRING_LGX");
                        break;
                    default:
                        popupMessage("not yet implemented");
                    }
                    break;
                // case 2:
                //     testModbus();
                //     break;
                default:
                    popupMessage("code test here...");
                    break;
                }
            }

            break;
            case KEY_F(4):
                if(f.label == "Tagname:") {
                    f.options.clear();
                }
                break;
            case KEY_F(5):
                if (f.editable) {
                    f.value.clear();
                    cursor_pos = 0;
                }
                break;
            case KEY_F(6):
                clearSection3Fields();
                break;

            case KEY_LEFT: if(f.editable && (f.options.empty() || f.label == "Tagname:") && cursor_pos>0) cursor_pos--; break;
            case KEY_RIGHT: if(f.editable && (f.options.empty() || f.label == "Tagname:") && cursor_pos<(int)f.value.size()) cursor_pos++; break;

            case KEY_BACKSPACE:
            case 127: if(f.editable && (f.options.empty() || f.label == "Tagname:") && cursor_pos>0){ f.value.erase(cursor_pos-1,1); cursor_pos--; } break;
            case KEY_DC: if(f.editable && (f.options.empty() || f.label == "Tagname:") && cursor_pos<(int)f.value.size()) f.value.erase(cursor_pos,1); break;

            case 10: handleEnterKey(f); break;

            default:
                if (f.editable && (f.options.empty() || f.label == "Tagname:") && ch >= 32 && ch <= 126) {
                    // Only allow input if under width limit
                    if ((int)f.value.size() < f.width) {
                        f.value.insert(cursor_pos, 1, (char)ch);
                        cursor_pos++;
                    }
                }
                break;
            }

            if(&f == &sections[1]->fields[2]) checkCountChange();

            // Update Section 3 widths if type changed
            std::string current_type = sections[1]->fields[0].value;
            if(current_type != last_type) {
                last_type = current_type;
                updateSection3Widths();
            }
        }
    }

    void checkCountChange() {
        Field& countField = sections[1]->fields[2];
        int current_count = 0;

        try {
            current_count = std::stoi(countField.value);
        } catch(...) {
            current_count = 0;
        }

        if(current_count != last_count) {
            last_count = current_count;
            updateSection3FromCount();
        }
    }

    string getGateway()  const { return sections[0]->fields[0].value; }
    string getPath()     const { return sections[0]->fields[1].value; }
    string getProtocol() const { return sections[0]->fields[2].value; }
    string getCpu()      const { return sections[0]->fields[3].value; }
    string getType()     const { return sections[1]->fields[0].value; }
    string getTagname()  const { return sections[1]->fields[1].value; }
    int getCount() const {
        try { return stoi(sections[1]->fields[2].value); }
        catch(...) { return 0; }
    }

    vector<string> getSection3Values() const {
        vector<string> vals;
        for(auto &f: sections[2]->fields) vals.push_back(f.value);
        return vals;
    }

private:
    vector<Section*> sections;
    int act_sec, act_field;
    int cursor_pos, scroll;
    int rows, cols;
    int last_count = -1;

    // ---------- Formatting helpers (new) ----------
    const char* getFormatForType(const std::string &type) {
        if(type == "BIT") return "%d";
        if(type == "BOOL") return "%d";
        if(type == "INT8") return "%d";
        if(type == "INT16") return "%d";
        if(type == "INT32") return "%ld";
        if(type == "INT64") return "%lld";
        if(type == "UINT8") return "%u";
        if(type == "UINT16") return "%u";
        if(type == "UINT32") return "%lu";
        if(type == "UINT64") return "%llu";
        if(type == "REAL32") return "%g";
        if(type == "REAL64") return "%g";
        if(type == "STRING") return "%s";
        return "%s";
    }

    void loadTagHistory() {
        ifstream in(TAG_HISTORY_FILE);
        if(!in) return;

        Field &tagField = sections[1]->fields[1];
        string line;
        int i = 0;
        while(i++ < TAG_HISTORY_MAX && getline(in, line)) {
            tagField.options.push_back(line); // to end
            //tagField.options.insert(tagField.options.begin(), line); //to top
        }
        //while(tagField.options.size() > TAG_HISTORY_MAX) tagField.options.erase(tagField.options.begin());

    }

    void saveTagHistory() {
        ofstream out(TAG_HISTORY_FILE);
        if(!out) return;

        Field &tagField = sections[1]->fields[1];
        for(const auto &tag : tagField.options) {
            out << tag << "\n";
        }
    }

    void storeCurrentTag() {
        Field &tagField = sections[1]->fields[1];
        if(!tagField.value.empty()) {
            //tagField.options.push_back(tagField.value); //to end
            tagField.options.insert(tagField.options.begin(), tagField.value); //to top

            // while(tagField.options.size() > TAG_HISTORY_MAX) tagField.options.erase(tagField.options.begin());  //from beginning
            while(tagField.options.size() > TAG_HISTORY_MAX) tagField.options.pop_back(); // from end

            tagField.options = removeDuplicatesPreserveOrder(tagField.options);

            // order the vector
            // set<string> s(tagField.options.begin(), tagField.options.end());
            // tagField.options.assign(s.begin(), s.end());

            // ofstream out(TAG_HISTORY_FILE, ios::app);
            // if(out) out << tagField.value << "\n";
            saveTagHistory();
        }
    }

    void formatValueForType(const std::string &type, const std::string &raw, char *out, size_t outSize) {
        if(type == "REAL32") {
            char *endptr = nullptr;
            errno = 0;
            float v = std::strtof(raw.c_str(), &endptr);
            if(endptr == raw.c_str() || errno != 0) v = 0.0f;
            std::snprintf(out, outSize, getFormatForType(type), v);
            return;
        }

        if(type == "REAL64") {
            char *endptr = nullptr;
            errno = 0;
            double v = std::strtod(raw.c_str(), &endptr);
            if(endptr == raw.c_str() || errno != 0) v = 0.0;
            std::snprintf(out, outSize, getFormatForType(type), v);
            return;
        }

        if(type == "INT8" || type == "INT16") {
            char *endptr = nullptr;
            errno = 0;
            long v = std::strtol(raw.c_str(), &endptr, 10);
            if(endptr == raw.c_str() || errno != 0) v = 0;
            std::snprintf(out, outSize, getFormatForType(type), static_cast<int>(v));
            return;
        }

        if(type == "INT32") {
            char *endptr = nullptr;
            errno = 0;
            long v = std::strtol(raw.c_str(), &endptr, 10);
            if(endptr == raw.c_str() || errno != 0) v = 0;
            std::snprintf(out, outSize, getFormatForType(type), static_cast<long int>(v));
            return;
        }

        if(type == "INT64") {
            char *endptr = nullptr;
            errno = 0;
            long long v = std::strtoll(raw.c_str(), &endptr, 10);
            if(endptr == raw.c_str() || errno != 0) v = 0;
            std::snprintf(out, outSize, getFormatForType(type), static_cast<long long int>(v));
            return;
        }

        if(type == "UINT8" || type == "UINT16") {
            char *endptr = nullptr;
            errno = 0;
            unsigned long v = std::strtoul(raw.c_str(), &endptr, 10);
            if(endptr == raw.c_str() || errno != 0) v = 0;
            std::snprintf(out, outSize, getFormatForType(type), static_cast<unsigned int>(v));
            return;
        }

        if(type == "UINT32") {
            char *endptr = nullptr;
            errno = 0;
            unsigned long v = std::strtoul(raw.c_str(), &endptr, 10);
            if(endptr == raw.c_str() || errno != 0) v = 0;
            std::snprintf(out, outSize, getFormatForType(type), static_cast<unsigned long int>(v));
            return;
        }

        if(type == "UINT64") {
            char *endptr = nullptr;
            errno = 0;
            unsigned long long v = std::strtoull(raw.c_str(), &endptr, 10);
            if(endptr == raw.c_str() || errno != 0) v = 0;
            std::snprintf(out, outSize, getFormatForType(type), static_cast<unsigned long long int>(v));
            return;
        }

        if(type == "BIT" || type == "BOOL") {
            char *endptr = nullptr;
            errno = 0;
            long v = std::strtol(raw.c_str(), &endptr, 10);
            if(endptr == raw.c_str() || errno != 0) v = 0;
            std::snprintf(out, outSize, getFormatForType(type), (int)(v ? 1 : 0));
            return;
        }

        // STRING or fallback
        std::snprintf(out, outSize, getFormatForType(type), raw.c_str());
    }
    // ---------- end formatting helpers ----------

    void updateSection3FromCount() {
        Section* sec2 = sections[1];
        int count = max(0, atoi(sec2->fields[2].value.c_str()));
        vector<string> seq(count, "0");
        updateSection3FromVector(seq);
    }

    // ---------- Helper function ----------
    int getFieldWidth(const std::string& fieldType, int maxWidth) {
        int width = 15;  // default width

        // Adjust widths per type later if desired
        if (fieldType == "BIT") width = 1;
        else if (fieldType == "BOOL") width = 1;
        else if (fieldType == "INT8") width = 4;
        else if (fieldType == "INT16") width = 6;
        else if (fieldType == "INT32") width = 10;
        else if (fieldType == "REAL32") width = 11;
        else if (fieldType == "REAL64") width = 18;
        //else if (fieldType == "STRING") width = maxWidth;
        else width = maxWidth;

        // Never exceed the screen width minus borders/padding
        if (width > maxWidth) width = maxWidth;
        return width;
    }

    // ---------- Update Section 3 widths ----------
    void updateSection3Widths() {
        Section* sec3 = sections[2];
        Section* sec2 = sections[1];       // Section 2 contains field types
        int maxFieldWidth = cols - 2 - 6;  // terminal width minus border and padding
        std::string typeStr = "REAL32";
        if(!sec2->fields.empty()) typeStr = sec2->fields[0].value;
        int field_width = getFieldWidth(typeStr, maxFieldWidth);
        for(auto &f : sec3->fields) f.width = field_width;
    }

    // ---------- Section 3 update ----------
    void updateSection3FromVector(const std::vector<std::string>& values, bool forceUpdate = false) {
        Section* sec3 = sections[2];
        Section* sec2 = sections[1]; // Section 2 contains field types
        int count = (int)values.size();
        scroll = 0;
        setFieldValue("Count:", to_string(values.size()));

        int maxFieldWidth = cols - 2 - 6;  // terminal width minus border and padding

        while ((int)sec3->fields.size() < count) {
            int idx = (int)sec3->fields.size();

            // Determine type for this field, fallback to REAL if Section 2 type not available
            std::string typeStr = "REAL32";
            if (!sec2->fields.empty()) typeStr = sec2->fields[0].value;

            int field_width = getFieldWidth(typeStr, maxFieldWidth);

            sec3->fields.push_back({"", "", 2, sec3->top + 1 + idx, field_width, true, {}});
        }

        if ((int)sec3->fields.size() > count)
            sec3->fields.resize(count);

        for (int i = 0; i < count; ++i) {
            // Update width dynamically in case screen changed or type changed
            std::string typeStr = "REAL32";
            if (!sec2->fields.empty()) typeStr = sec2->fields[0].value;
            sec3->fields[i].width = getFieldWidth(typeStr, maxFieldWidth);

            char formatted[256];
            formatValueForType(typeStr, values[i], formatted, sizeof(formatted));
            sec3->fields[i].value = formatted;
            sec3->fields[i].y = sec3->top + 1 + i;
        }

        if(forceUpdate) {
            scroll = 0;
            updateUI();
        }
    }

    void clearSection3Fields() {
        Section* sec3 = sections[2];
        for (auto &f : sec3->fields) {
            if (f.editable) f.value.clear();
        }
        cursor_pos = 0;
    }

    void drawBorders() {
        Section &sec1=*sections[0], &sec2=*sections[1], &sec3=*sections[2];
        mvaddch(sec1.top,0,ACS_ULCORNER); mvhline(sec1.top,1,ACS_HLINE,cols-2); mvaddch(sec1.top,cols-1,ACS_URCORNER);
        for(int r=sec1.top+1;r<sec1.bottom;++r){ mvaddch(r,0,ACS_VLINE); mvaddch(r,cols-1,ACS_VLINE); }
        mvaddch(sec1.bottom,0,ACS_LTEE); mvhline(sec1.bottom,1,ACS_HLINE,cols-2); mvaddch(sec1.bottom,cols-1,ACS_RTEE);

        for(int r=sec2.top+1;r<sec2.bottom;++r){ mvaddch(r,0,ACS_VLINE); mvaddch(r,cols-1,ACS_VLINE); }
        mvaddch(sec2.bottom,0,ACS_LTEE); mvhline(sec2.bottom,1,ACS_HLINE,cols-2); mvaddch(sec2.bottom,cols-1,ACS_RTEE);

        for(int r=sec3.top+1;r<sec3.bottom;++r){ mvaddch(r,0,ACS_VLINE); mvaddch(r,cols-1,ACS_VLINE); }
        mvaddch(sec3.bottom,0,ACS_LLCORNER); mvhline(sec3.bottom,1,ACS_HLINE,cols-2); mvaddch(sec3.bottom,cols-1,ACS_LRCORNER);
    }

    void drawSection3Fields() {
        Section* sec3 = sections[2];
        int vis = sec3->bottom - sec3->top - 1;
        int total = (int)sec3->fields.size();
        if(act_sec == 2){
            if(act_field < scroll) scroll = act_field;
            if(act_field >= scroll + vis) scroll = act_field - vis + 1;
        }
        for(int i=0;i<vis;++i){
            int idx = i + scroll;
            if(idx >= total) break;
            Field &f = sec3->fields[idx];
            if(act_sec == 2 && idx == act_field) attron(A_REVERSE);

            // Format the field value according to Section 2 Type, except while editing
            char formatted[256];
            std::string typeStr = "REAL32";
            if(!sections[1]->fields.empty()) typeStr = sections[1]->fields[0].value;

            if(act_sec == 2 && idx == act_field) {
                // Show raw value while editing current field
                std::snprintf(formatted, sizeof(formatted), "%s", f.value.c_str());
            } else {
                // Show formatted representation
                formatValueForType(typeStr, f.value, formatted, sizeof(formatted));
                f.value = formatted;
            }

            mvprintw(sec3->top + 1 + i, f.x, "%02d:%-*s", idx+1, f.width, formatted);
            if(act_sec == 2 && idx == act_field) attroff(A_REVERSE);
        }
    }

    void drawSections12Fields() {
        for(int s=0; s<2; ++s){
            Section *sec = sections[s];
            for(size_t i=0; i<sec->fields.size(); ++i){
                Field &f = sec->fields[i];
                mvprintw(f.y, f.x, "%s", f.label.c_str());
                bool active = (int)s==act_sec && (int)i==act_field;
                if(f.editable || (!f.editable && (f.label=="[Read]" || f.label=="[Write]"))) {
                    if(active) attron(A_REVERSE);
                    if(!f.editable) mvprintw(f.y,f.x,"%s",f.label.c_str());
                    else mvprintw(f.y,f.x+f.label.size(),"%-*s",f.width,f.value.c_str());
                    if(active) attroff(A_REVERSE);
                }
            }
        }
    }

    void setCursor() {
        Field &af = sections[act_sec]->fields[act_field];

        if (af.editable && (af.options.empty() || af.label == "Tagname:")) {
            int cursor_y = af.y;
            int cursor_x = af.x + af.label.size();
            if(cursor_pos > (int)af.value.size()) cursor_pos = af.value.size();
            cursor_x += cursor_pos;

            if(act_sec == 2) cursor_x += 3;

            move(cursor_y, cursor_x);
            curs_set(1);
        } else curs_set(0);
    }

    void moveFieldUp() { storeTagOnLeave(); Section *sec = sections[act_sec]; do { act_field = (act_field-1+sec->fields.size())%sec->fields.size(); } while(!sec->fields[act_field].editable && sec->fields[act_field].label.empty()); cursor_pos=sec->fields[act_field].value.size(); }
    void moveFieldDown() { storeTagOnLeave(); Section *sec = sections[act_sec]; do { act_field=(act_field+1)%sec->fields.size(); } while(!sec->fields[act_field].editable && sec->fields[act_field].label.empty()); cursor_pos=sec->fields[act_field].value.size(); }
    void moveSectionPrev() { storeTagOnLeave(); sections[act_sec]->last_active=act_field; act_sec=(act_sec-1+(int)sections.size())%sections.size(); act_field=sections[act_sec]->last_active; cursor_pos=sections[act_sec]->fields[act_field].value.size(); }
    void moveSectionNext() { storeTagOnLeave(); sections[act_sec]->last_active=act_field; act_sec=(act_sec+1)%sections.size(); act_field=sections[act_sec]->last_active; cursor_pos=sections[act_sec]->fields[act_field].value.size(); }

    void storeTagOnLeave() {
        if(act_sec == 1 && act_field == 1) { // Only Tagname field
            storeCurrentTag();
        }
    }

    void pageUpSection3() {
        Section* sec3 = sections[2];   // always Section 3
        int total = sec3->fields.size();
        int vis = sec3->bottom - sec3->top - 1;

        if (total <= vis) return; // no scrolling needed

        if (scroll <= 0) {
            // wrap to bottom
            scroll = max(0, total - vis);
        } else {
            scroll -= vis;
            if (scroll < 0) scroll = 0;
        }
    }

    void pageDownSection3() {
        Section* sec3 = sections[2];
        int total = sec3->fields.size();
        int vis = sec3->bottom - sec3->top - 1;

        if (total <= vis) return; // no scrolling needed

        if (scroll >= total - vis) {
            // wrap to top
            scroll = 0;
        } else {
            scroll += vis;
            if (scroll > total - vis)
                scroll = max(0, total - vis);
        }
    }

    // Set the Tagname field and update the display immediately
    void setTagname(const std::string& newTagname) {
        Section* sec2 = sections[1];
        Field& tagField = sec2->fields[1]; // "Tagname:" field
        tagField.value = newTagname;
        cursor_pos = tagField.value.size(); // move cursor to end

        // Add to tag history
        //storeCurrentTag();

        // Refresh UI to show the updated tagname
        updateUI();
    }
    void setFieldValue(const std::string& fieldLabel, const std::string& newValue, bool forceUpdate = false) {
        for (auto* sec : sections) {
            for (auto& f : sec->fields) {
                if (f.label == fieldLabel) {
                    f.value = newValue;

                    // // Special handling for certain fields
                    // if (fieldLabel == "Tagname:") {
                    //     cursor_pos = f.value.size();
                    //     storeCurrentTag();          // Save tag history
                    // }
                    if (fieldLabel == "Type:") {
                        //    cursor_pos = f.value.size();
                        updateSection3Widths();    // Adjust Section 3 widths when type changes
                    }

                    if(forceUpdate) updateUI(); // redraw screen with new value
                    return; // stop after updating first match
                }
            }
        }
    }

    // uint16_t getTypeByteCount(string type) {
    //     if(type == "") {
    //         raise(SIGTRAP);
    //         return -1;
    //     } else if(type == "BIT") {
    //         return 1;
    //     } else if(type == "SINT") {
    //         return 2;
    //     } else if(type == "INT") {
    //         return 3;
    //     } else if(type == "DINT") {
    //         return 4;
    //     } else if(type == "LINT") {
    //         return 8;
    //     } else if(type == "STRING") {
    //         return 1;
    //     } else if(type == "STRING_LGX") {
    //         return 1;
    //     } else {
    //         raise(SIGTRAP);
    //         return -2;
    //     }
    // }

    TagEntry expandTagentry(TagEntry e, string name, string type, uint16_t count = 1) {
        return { "",
                e.name + "." + name,
                e.parentName,
                0,
                getTagTypeCode(type),
                0, //getTypeByteCount(type),
                count,
                0,
                0 };
    }

    int tobefleshed = 0;
    vector<string> to_flesh;
    void expand_tags(vector<TagEntry> &tagentries) {
        vector<TagEntry> to_add;
        vector<int> to_remove;
        string type;
        auto &t = tagentries;
        for (int i = 0; i < tagentries.size(); i++) {
            auto& e = tagentries[i];
            // if (e.name.find("Local:12:I") != std::string::npos) {
            //     raise(SIGTRAP);
            // }
            type = getTagType(e.type);
            tobefleshed++;

            if(type == "" || type == ".UNKNOWN.") {
                tobefleshed--;
                to_remove.push_back(i);
                to_flesh.push_back(e.name);
                //if(type == "") { raise(SIGTRAP); }
                //raise(SIGTRAP);

                ///////////////////////////////////
                /// haven't yet looked up in PLC
                ///////////////////////////////////
                // __DEFVAL_00000A75
                // SR1_SELECT
                // SR1_RUNTIME
                // SR1_LOAD
                // SR1_COMMANDS
                // SR1_AUTOSTART
                // SE_CMDS_FROM_T10
                // SE_BLDG_STAT
                // R9S9_STATUS
                // R9S8_STATUS
                // R9S7_STATUS
                // R9S6_STATUS
                // R9S5_STATUS
                // R9S4_STATUS
                // R9S3_STATUS
                // R9S1_STATUS
                // R9S12_STATUS
                // R9S12_CH_FAULT
                // JAC_BOOL
                // I_BUTTON
                // Local:9:O
                // Local:9:I
                // Local:8:O
                // Local:8:I
                ///////////////////////////////////
                ///
            } else if(type == "Timer") {
                tobefleshed--;
                to_remove.push_back(i);
                to_add.push_back(expandTagentry(e, "PRE", "DINT"));
                to_add.push_back(expandTagentry(e, "ACC", "DINT"));
                to_add.push_back(expandTagentry(e, "EN",  "BIT"));
                to_add.push_back(expandTagentry(e, "TT",  "BIT"));
                to_add.push_back(expandTagentry(e, "DN",  "BIT"));
            } else if(type == "MESSAGE") {
                tobefleshed--;
                to_remove.push_back(i);
                to_add.push_back(expandTagentry(e, "Flags",              "INT"));
                to_add.push_back(expandTagentry(e, "EW",                 "BIT"));
                to_add.push_back(expandTagentry(e, "ER",                 "BIT"));
                to_add.push_back(expandTagentry(e, "DN",                 "BIT"));
                to_add.push_back(expandTagentry(e, "ST",                 "BIT"));
                to_add.push_back(expandTagentry(e, "EN",                 "BIT"));
                to_add.push_back(expandTagentry(e, "TO",                 "BIT"));
                to_add.push_back(expandTagentry(e, "EN_CC",              "BIT"));
                to_add.push_back(expandTagentry(e, "ERR",                "INT"));
                to_add.push_back(expandTagentry(e, "EXERR",              "DINT"));
                to_add.push_back(expandTagentry(e, "ERR_SRC",            "SINT"));
                to_add.push_back(expandTagentry(e, "DN_LEN",             "INT"));
                to_add.push_back(expandTagentry(e, "REQ_LEN",            "INT"));
                to_add.push_back(expandTagentry(e, "DestinationLink",    "INT"));
                to_add.push_back(expandTagentry(e, "DestinationNode",    "INT"));
                to_add.push_back(expandTagentry(e, "SourceLink",         "INT"));
                to_add.push_back(expandTagentry(e, "Class",              "INT"));
                to_add.push_back(expandTagentry(e, "Attribute",          "INT"));
                to_add.push_back(expandTagentry(e, "Instance",           "DINT"));
                to_add.push_back(expandTagentry(e, "LocalIndex",         "DINT"));
                to_add.push_back(expandTagentry(e, "Channel",            "SINT"));
                to_add.push_back(expandTagentry(e, "Rack",               "SINT"));
                to_add.push_back(expandTagentry(e, "Group",              "SINT"));
                to_add.push_back(expandTagentry(e, "Slot",               "SINT"));
                to_add.push_back(expandTagentry(e, "Path",               "STRING_LGX"));
                to_add.push_back(expandTagentry(e, "RemoteIndex",        "DINT"));
                to_add.push_back(expandTagentry(e, "RemoteElement",      "STRING_LGX"));
                to_add.push_back(expandTagentry(e, "UnconnectedTimeout", "DINT"));
                to_add.push_back(expandTagentry(e, "ConnectionRate",     "DINT"));
                to_add.push_back(expandTagentry(e, "TimeoutMultiplier",  "SINT"));
            } else if(type == "COUNTER") {
                tobefleshed--;
                to_remove.push_back(i);
                to_add.push_back(expandTagentry(e, "PRE", "DINT"));
                to_add.push_back(expandTagentry(e, "ACC", "DINT"));
                to_add.push_back(expandTagentry(e, "CU",  "BIT"));
                to_add.push_back(expandTagentry(e, "CD",  "BIT"));
                to_add.push_back(expandTagentry(e, "OV",  "BIT"));
                to_add.push_back(expandTagentry(e, "UN",  "BIT"));
            } else if(type == "ALARM_DIGITAL") {
                tobefleshed--;
                to_remove.push_back(i);
                to_add.push_back(expandTagentry(e, "EnableIn",  "BIT"));
                to_add.push_back(expandTagentry(e, "In",  "BIT"));
                to_add.push_back(expandTagentry(e, "InFault",  "BIT"));
                to_add.push_back(expandTagentry(e, "Condition",  "BIT"));
                to_add.push_back(expandTagentry(e, "AckRequired",  "BIT"));
                to_add.push_back(expandTagentry(e, "Latched",  "BIT"));
                to_add.push_back(expandTagentry(e, "ProgAck",  "BIT"));
                to_add.push_back(expandTagentry(e, "OperAck",  "BIT"));
                to_add.push_back(expandTagentry(e, "ProgReset",  "BIT"));
                to_add.push_back(expandTagentry(e, "OperReset",  "BIT"));
                to_add.push_back(expandTagentry(e, "ProgSuppress",  "BIT"));
                to_add.push_back(expandTagentry(e, "OperSuppress",  "BIT"));
                to_add.push_back(expandTagentry(e, "ProgUnsuppress",  "BIT"));
                to_add.push_back(expandTagentry(e, "OperUnsuppress",  "BIT"));
                to_add.push_back(expandTagentry(e, "OperShelve",  "BIT"));
                to_add.push_back(expandTagentry(e, "ProgUnshelve",  "BIT"));
                to_add.push_back(expandTagentry(e, "OperUnshelve",  "BIT"));
                to_add.push_back(expandTagentry(e, "ProgDisable",  "BIT"));
                to_add.push_back(expandTagentry(e, "OperDisable",  "BIT"));
                to_add.push_back(expandTagentry(e, "ProgEnable",  "BIT"));
                to_add.push_back(expandTagentry(e, "OperEnable",  "BIT"));
                to_add.push_back(expandTagentry(e, "AlarmCountReset",  "BIT"));
                to_add.push_back(expandTagentry(e, "UseProgTime",  "BIT"));
                to_add.push_back(expandTagentry(e, "ProgTime",         "LINT"));
            } else if(type == "XFR_TYPE") {
                tobefleshed--;
                to_remove.push_back(i);
                for(int i=0; i<e.elem_count; i++) {
                    TagEntry ec = e;
                    ec.name = e.name + "[" + to_string(i) + "]";
                    to_add.push_back(expandTagentry(ec, "CONV_XFR", "SINT"));
                    to_add.push_back(expandTagentry(ec, "ITLK_XFR", "SINT"));
                    to_add.push_back(expandTagentry(ec, "ALLOCATED_UNIT", "DINT"));
                    to_add.push_back(expandTagentry(ec, "COMMAND", "INT"));
                    to_add.push_back(expandTagentry(ec, "RUNTIME.HOURS", "DINT"));
                    to_add.push_back(expandTagentry(ec, "RUNTIME.MINUTES", "INT"));
                    to_add.push_back(expandTagentry(ec, "WT.RATE", "DINT"));
                    to_add.push_back(expandTagentry(ec, "WT.SCALE", "DINT"));
                    to_add.push_back(expandTagentry(ec, "WT.TOTAL_TONNAGE", "DINT"));
                    to_add.push_back(expandTagentry(ec, "FLOP_GATE", "SINT"));
                    to_add.push_back(expandTagentry(ec, "T10_CONV_STAT", "SINT"));
                    to_add.push_back(expandTagentry(ec, "MCT.MCT_00", "INT"));
                    to_add.push_back(expandTagentry(ec, "MCT.MCT_01", "INT"));
                    to_add.push_back(expandTagentry(ec, "MCT.MCT_02", "INT"));
                    to_add.push_back(expandTagentry(ec, "MCT.MCT_03", "INT"));
                    to_add.push_back(expandTagentry(ec, "LIGHTS", "DINT", 2));
                }
            } else if(type == "TIME_OF_DAY") {
                to_remove.push_back(i);
            } else if(type == "FULL_STRUCT") {
                to_remove.push_back(i);
            } else if(type == "RADIO_COMM_TYPE") {
                to_remove.push_back(i);
            } else if(type == "CONVEYOR_TYPE") {
                to_remove.push_back(i);
            } else if(type == "PM_XFR_STATUS_TYPE") {
                to_remove.push_back(i);
            } else if(type == "I_BUTTON") {
                to_remove.push_back(i);
            } else if(type == "SCALE_TYPE") {
                to_remove.push_back(i);
            } else if(type == "PM_XFR_STATUS_TYPE") {
                to_remove.push_back(i);
            } else if(type == "TIMESTAMP2_TYPE") {
                to_remove.push_back(i);

                // 1756 IO
            } else if(type == "1756_DI_C") {
                tobefleshed--;
                to_remove.push_back(i);
                to_add.push_back(expandTagentry(e, "DiagCOSDisable", "BIT"));
                to_add.push_back(expandTagentry(e, "FilterOffOn_0_7", "SINT"));
                to_add.push_back(expandTagentry(e, "FilterOnOff_0_7", "SINT"));
                to_add.push_back(expandTagentry(e, "FilterOffOn_8_15", "SINT"));
                to_add.push_back(expandTagentry(e, "FilterOnOff_8_15", "SINT"));
                to_add.push_back(expandTagentry(e, "FilterOffOn_16_23", "SINT"));
                to_add.push_back(expandTagentry(e, "FilterOnOff_16_23", "SINT"));
                to_add.push_back(expandTagentry(e, "FilterOffOn_24_31", "SINT"));
                to_add.push_back(expandTagentry(e, "FilterOnOff_24_31", "SINT"));
                to_add.push_back(expandTagentry(e, "COSOnOffEn", "DINT"));
                to_add.push_back(expandTagentry(e, "COSOffOnEn", "DINT"));
            } else if(type == "1756_DI_I") {
                tobefleshed--;
                to_remove.push_back(i);
                to_add.push_back(expandTagentry(e, "Fault", "DINT"));
                to_add.push_back(expandTagentry(e, "Data", "DINT"));
            } else if(type == "1756_DO_C") {
                tobefleshed--;
                to_remove.push_back(i);
                to_add.push_back(expandTagentry(e, "ProgToFaultEn", "BIT"));
                to_add.push_back(expandTagentry(e, "FaultMode", "DINT"));
                to_add.push_back(expandTagentry(e, "FaultValue", "DINT"));
                to_add.push_back(expandTagentry(e, "ProgMode", "DINT"));
                to_add.push_back(expandTagentry(e, "ProgValue", "DINT"));
            } else if(type == "1756_IF16_C") {
                tobefleshed--;
                to_remove.push_back(i);
                to_add.push_back(expandTagentry(e, "ModuleFilter", "SINT"));
                to_add.push_back(expandTagentry(e, "RealTimeSample", "INT"));
                for(int i=0; i<16; i++) {
                    TagEntry ec = e;
                    string prefix = "Ch" + to_string(i) + "Config.";
                    to_add.push_back(expandTagentry(e, prefix + "RangeType", "SINT"));
                    to_add.push_back(expandTagentry(e, prefix + "DigitalFilter", "INT"));
                    to_add.push_back(expandTagentry(e, prefix + "LowSignal", "REAL"));
                    to_add.push_back(expandTagentry(e, prefix + "HighSignal", "REAL"));
                    to_add.push_back(expandTagentry(e, prefix + "LowEngineering", "REAL"));
                    to_add.push_back(expandTagentry(e, prefix + "HighEngineering", "REAL"));
                    to_add.push_back(expandTagentry(e, prefix + "CalBias", "REAL"));
                }
            } else if(type == "1756_IF16_I") {
                to_remove.push_back(i);

                // not strictly data
            } else if(type == "Routine") {
                to_remove.push_back(i);
            } else if(type == "Program") {
                to_remove.push_back(i);
            } else if(type == "Task") {
                to_remove.push_back(i);

                // unlocated
            } else if(type == "CXN_STANDARD") {
                to_remove.push_back(i);
            } else if(type == "MAP_R999") {
                to_remove.push_back(i);
            } else {
                // must be base type
                if (string("BIT SINT INT DINT LINT REAL STRING_LGX").find(type) == std::string::npos) {
                    TagEntry x = e;
                    raise(SIGTRAP);
                }
            }
        }

        sort(to_remove.rbegin(), to_remove.rend());
        for (int i : to_remove) {
            tagentries[i] = tagentries.back();
            tagentries.pop_back();
        }
        int i = 0;
        for (auto &e : to_add) {
            tagentries.push_back(e);
            i++;
        }
    }

    string makeTagName(TagEntry te) {
        string tagname = "";

        if(te.parentName.size() > 0) tagname = te.parentName + ".";
        tagname += te.name;

        return tagname;
    }

    void showProgress(int i) {
        curs_set(0); // HIDE CURSOR

        Section* sec3 = sections[2];

        // Compute middle row of Section 3
        int mid_y = sec3->top + (sec3->bottom - sec3->top) / 2;
        int mid_x = cols / 2 - 20;  // center approx

        // Draw progress text
        mvprintw(mid_y, mid_x, "          %d          ", i);
        refresh();
    }
    void showProgressS(string &&s) {
        curs_set(0); // HIDE CURSOR

        Section* sec3 = sections[2];

        // Compute middle row of Section 3
        int mid_y = sec3->top + (sec3->bottom - sec3->top) / 2 - 1;
        int mid_x = cols / 2 - (s.size() + 37)/2;  // center approx

        // Draw progress text
        mvprintw(mid_y, mid_x, "          %s          ", s.c_str());
        refresh();
    }

    void profileTags(string types = "") {
        string gateway = getGateway();
        string path = getPath();
        string cpu = getCpu();
        string protocol = getProtocol();
        vector<string> tags_created, tags_read, fail_create, fail_read;
        int rc;

        vector<TagEntry> tagentries;
        const char* argv[3] = { (char*)"", gateway.c_str(), path.c_str() };
        list_tags(3, argv, tagentries);
        expand_tags(tagentries);

        // CREATE TAGS
        vector<TagEntry> teRead;
        vector<int> tags;

        //showProgressS(ssprintf("pre-creating the tags: %s", types.c_str()));
        showProgressS(ssprintf("pre-creating the%s tags", types.size() ? (" " + types).c_str() : "" ));

#define PROGRESS_GROUP 100
        //#define INVALID_TAG 32768
        int progress = 0;
        for (auto& e : tagentries) {
            string es, type = getTagType(e.type);
            //            e.instance_id = INVALID_TAG; // repurposing field as tag, initialize as invalid tag number
            if(types.size() > 0 && types.find(type))  {
                continue;
            } else {
                int i = 0;
            }

            // if (e.name.find("SE_BUILDING_STATUS[0].LIGHTS") != std::string::npos) {
            //     //raise(SIGTRAP);
            //     int i = 0;
            // }
            if(++progress % PROGRESS_GROUP == 0) showProgress(progress);

            e.name = makeTagName(e); // repurposing field as prefixed tagname
            //////////////////////////////////////////////////////
            if(true) e.elem_count = 1;  //////////////////////////
            //////////////////////////////////////////////////////
            string tagstring = buildTagstring(gateway, e.name, e.elem_count, path, cpu, protocol);
            int tag = createTag(es, tagstring);
            if(tag < 0) {
                es = ssprintf("Error creating tag %s (%X-%s): %s!\n", e.name.c_str(), e.type, type.c_str(), plc_tag_decode_error(tag));
                fail_create.push_back(es);
                continue;
            }
            else {
                //tags_created.push_back(ssprintf("%0.4x  %s", e.type, e.name.c_str()));
                tags_created.push_back(formatTagDisplay(e, 10));
            }
            e.instance_id = tag;     // repurposing field as tag number
            teRead.push_back(e);
        }
        //***************************
        sections[1]->fields[0].value = "STRING";
        updateSection3Widths();
        //***************************
        updateSection3FromVector(tags_created, true);
        //updateUI();

        int comm_count = 0;
        progress = 0;
        showProgressS(ssprintf("reading the%s tags", types.size() ? (" " + types).c_str() : "" ));

        auto start_us = high_resolution_clock::now();
        for (auto& e : teRead) {
            if(++progress % PROGRESS_GROUP == 0) showProgress(progress);
            // if (e.name.find("C24_MCT") != std::string::npos) {
            //     //raise(SIGTRAP);
            //     int i = 0;
            // }
            string es, type;
            int tag = e.instance_id;
            type = getTagType(e.type);
            //if(!type.size()) raise(SIGTRAP);
            //if(tag >= 0 && tag < INVALID_TAG && type.size() > 0) {
            comm_count++;
            rc = PLCTAG_STATUS_OK;
            if(type == "BIT") {
                vector<bool> values;
                rc = readNumericTag(es, tag, values);
            } else if(type == "SINT") {
                vector<int8_t> values;
                rc = readNumericTag(es, tag, values);
            } else if(type == "INT") {
                vector<int16_t> values;
                rc = readNumericTag(es, tag, values);
            } else if(type == "DINT") {
                vector<int32_t> values;
                rc = readNumericTag(es, tag, values);
            } else if(type == "LINT") {
                vector<int64_t> values;
                rc = readNumericTag(es, tag, values);
            } else if(type == "REAL") {
                vector<float_t> values;
                rc = readNumericTag(es, tag, values);
            } else if(type == "STRING_LGX") {
                vector<string> values;
                rc = readStringTag(es, tag, values);
            } else if(type == "BYTE") {
                comm_count--;
                raise(SIGTRAP);
            } else if(type == "DWORD") {
                comm_count--;
                raise(SIGTRAP);
            } else {
                comm_count--;
                raise(SIGTRAP);
            }

            if(rc != PLCTAG_STATUS_OK) {
                es = ssprintf("Error reading %s (%X-%s): %s!\n", e.name.c_str(), e.type, type.c_str(), plc_tag_decode_error(tag));
                fail_read.push_back(es);
            } else {
                //tags_read.push_back(e.name);
            }
        }
        auto end_us = high_resolution_clock::now();
        auto duration_us = duration_cast<microseconds>(end_us - start_us).count();
        auto duration_s  = static_cast<double>(duration_us) / 1000000.0;
        auto read_rate = std::round((comm_count/duration_s) * 100) / 100;

        progress = 0;
        showProgressS(ssprintf("destroying the%s tags", types.size() ? (" " + types).c_str() : "" ));
        for (auto& e : teRead) {
            if(++progress % PROGRESS_GROUP == 0) showProgress(progress);
            plc_tag_destroy(e.instance_id);
        }

        popupMessage(ssprintf("%s\n%.2f reads/s: %d reads in %dus / %.2fs  [%d]",
                              types.size() ? types.c_str() : "ALL TAGS", read_rate, comm_count, duration_us, duration_s, tobefleshed));

        //int z = to_flesh.size();
    }

    string formatTagDisplay(TagEntry e, int sTypeWidth = 18) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(4) << e.type << std::setfill(' ') << "-";

        ss << std::left << std::setw(sTypeWidth) << getTagType(e.type, "UNKN");

        ss << ":  ";

        if (!e.parentName.empty())
            ss << e.parentName << ".";
        ss << e.name;

        if (e.num_dimensions > 0) ss << "[" << e.dimensions[0] << "]";
        if (e.num_dimensions > 1) ss << "[" << e.dimensions[1] << "]";
        if (e.num_dimensions > 2) ss << "[" << e.dimensions[2] << "]";

        return ss.str();
    }

    void displayTags() {
        sections[1]->fields[0].value = "STRING";
        updateSection3Widths();

        vector<string> seq;
        string gateway = getGateway();
        string path = getPath();

        vector<TagEntry> tagentries;
        const char* argv[3] = { (char*)"", gateway.c_str(), path.c_str()};
        list_tags(3, argv, tagentries);

        for (const auto& e : tagentries) {
            seq.push_back(formatTagDisplay(e));
        }
        // sections[1]->fields[2].value = std::to_string(seq.size());
        // setFieldValue("Count:", to_string(seq.size()));
        updateSection3FromVector(seq);
    }

    template <typename T>
    int32_t readTagAndUpdateSection3(int32_t tag) {
        int32_t rc;
        string es;
        vector<T> values;

        if constexpr (std::is_same_v<T, string>) {
            rc = readStringTag(es, tag, values);
        } else {
            rc = readNumericTag(es, tag, values);
        }

        if(rc < 0) {
            popupMessage(es);
            return rc;
        }

        vector<string> seq;
        for(int i=0; i<values.size(); ++i) {
            if constexpr (std::is_same_v<T, string>) {
                seq.push_back(values[i]);
            } else {
                seq.push_back(to_string(values[i]));
            }
        }
        updateSection3FromVector(seq);

        return rc;
    }

    template <typename T>
    int32_t writeTagFromSection3(int32_t tag) {
        int32_t rc;
        string es;

        if constexpr (std::is_same_v<T, string>) {
            vector<T> values = getSection3Values();
            rc = writeStringTag(es, tag, values);
        } else {
            vector<T> values = toNumericVector<T>(getSection3Values());
            rc = writeNumericTag(es, tag, values);
        }

        if(rc < 0) popupMessage(es);

        return rc;
    }

    void handleEnterKey(Field &f) {
        string ftype = getType();
        int cnt = getCount();
        string es;

        if(false) {
            //PLACEHOLDER
            //popupMessage(to_string(rc));
        } else  if(!f.editable && (f.label=="[Read]" || f.label=="[Write]")) {
            string tagstring = buildTagstring(getGateway(), getTagname(), cnt, getPath(), getCpu(), getProtocol());
            int32_t tag = createTag(es, tagstring);
            if(tag < 0) {
                popupMessage(es);
                goto end_case;
            }

            if(f.label == "[Read]") {
                if("INT8" == ftype || "BOOL" == ftype) {
                    readTagAndUpdateSection3<int8_t>(tag);
                } else if("INT16" == ftype) {
                    readTagAndUpdateSection3<int16_t>(tag);
                } else if("INT32" == ftype) {
                    readTagAndUpdateSection3<int32_t>(tag);
                } else if("INT64" == ftype) {
                    readTagAndUpdateSection3<int64_t>(tag);
                } else if("UINT8" == ftype) {
                    readTagAndUpdateSection3<uint8_t>(tag);
                } else if("UINT16" == ftype) {
                    readTagAndUpdateSection3<uint16_t>(tag);
                } else if("UINT32" == ftype) {
                    readTagAndUpdateSection3<uint32_t>(tag);
                } else if("UINT64" == ftype) {
                    readTagAndUpdateSection3<uint64_t>(tag);
                } else if("REAL32" == ftype) {
                    readTagAndUpdateSection3<float_t>(tag);
                } else if("REAL64" == ftype) {
                    readTagAndUpdateSection3<double_t>(tag);
                } else if("BIT" == ftype) {
                    readTagAndUpdateSection3<bool>(tag);
                } else if("STRING" == ftype) {
                    readTagAndUpdateSection3<string>(tag);
                }
            } else if(f.label == "[Write]") {
                if("INT8" == ftype || "BOOL" == ftype) {
                    writeTagFromSection3<int8_t>(tag);
                } else if("INT16" == ftype) {
                    writeTagFromSection3<int16_t>(tag);
                } else if("INT32" == ftype) {
                    writeTagFromSection3<int32_t>(tag);
                } else if("INT64" == ftype) {
                    writeTagFromSection3<int64_t>(tag);
                } else if("UINT8" == ftype) {
                    writeTagFromSection3<uint8_t>(tag);
                } else if("UINT16" == ftype) {
                    writeTagFromSection3<uint16_t>(tag);
                } else if("UINT32" == ftype) {
                    writeTagFromSection3<uint32_t>(tag);
                } else if("UINT64" == ftype) {
                    writeTagFromSection3<uint64_t>(tag);
                } else if("REAL32" == ftype) {
                    writeTagFromSection3<float_t>(tag);
                } else if("REAL64" == ftype) {
                    writeTagFromSection3<double_t>(tag);
                } else if("BIT" == ftype) {
                    writeTagFromSection3<bool>(tag);
                } else if("STRING" == ftype) {
                    writeTagFromSection3<string>(tag);
                }
            }
            plc_tag_destroy(tag);
        end_case:;
        }
    }

    void popupMessage(const std::string& message) {

        curs_set(0); // HIDE CURSOR

        // ---- Split message into lines ----
        std::vector<std::string> lines;
        std::string current;
        std::istringstream iss(message);
        while (std::getline(iss, current)) {
            lines.push_back(current);
        }
        if (lines.empty()) lines.push_back("");

        // ---- Determine window size ----
        int max_len = 0;
        for (const auto& line : lines) {
            max_len = std::max(max_len, (int)line.size());
        }

        int padding = 6;
        int win_w = max_len + padding;
        if (win_w < 30) win_w = 30;
        if (win_w > COLS - 4) win_w = COLS - 4;

        int win_h = (int)lines.size() + 4;   // 2 border + lines + hint + some spacing
        if (win_h > LINES - 4) win_h = LINES - 4;

        int win_y = (LINES - win_h) / 2;
        int win_x = (COLS - win_w) / 2;

        // ---- Create window ----
        WINDOW* win = newwin(win_h, win_w, win_y, win_x);
        box(win, 0, 0);
        keypad(win, TRUE);

        // ---- Print each line centered ----
        int start_y = 2;
        for (size_t i = 0; i < lines.size(); ++i) {
            int x = (win_w - (int)lines[i].size()) / 2;
            mvwprintw(win, start_y + i, x, "%s", lines[i].c_str());
        }

        // ---- Display ----
        wrefresh(win);
        wgetch(win);
        delwin(win);

        touchwin(stdscr);
        refresh();

        curs_set(1); // UNHIDE CURSOR
    }

    void popupSelectOptions(Field &f) {
        if (f.options.empty()) return;

        int total_opts = (int)f.options.size();
        int desired_h = total_opts + 3;            // label + border padding
        int max_h = LINES - 2;                     // ensure fits on screen
        int win_h = std::min(desired_h, max_h);

        int max_len = (int)f.label.size();
        for (auto &opt : f.options)
            max_len = std::max(max_len, (int)opt.size());

        int win_w = std::max(20, max_len + 6);
        win_w = std::min(win_w, COLS - 4);

        int win_y = std::max(0, (LINES - win_h) / 2);
        int win_x = std::max(0, (COLS - win_w) / 2);

        WINDOW* win = newwin(win_h, win_w, win_y, win_x);
        keypad(win, TRUE);

        // --- Find initial highlight ---
        int highlight = 0;
        for (size_t i = 0; i < f.options.size(); ++i)
            if (f.options[i] == f.value)
                highlight = (int)i;

        // First visible option index (scroll position)
        int start = 0;

        // How many options fit inside the window?
        int visible_count = win_h - 3;  // 1 label + 2 borders

        auto draw = [&]() {
            werase(win);
            box(win, 0, 0);

            mvwprintw(win, 1, 2, "%s", f.label.c_str());

            // adjust scroll
            if (highlight < start) start = highlight;
            if (highlight >= start + visible_count)
                start = highlight - visible_count + 1;

            // draw visible options
            for (int i = 0; i < visible_count && (start + i) < total_opts; i++) {
                int idx = start + i;
                if (idx == highlight) wattron(win, A_REVERSE);
                mvwprintw(win, 2 + i, 2, "%s", f.options[idx].c_str());
                if (idx == highlight) wattroff(win, A_REVERSE);
            }

            // scroll indicators
            if (start > 0)
                mvwprintw(win, 2, win_w - 2, "^");
            if (start + visible_count < total_opts)
                mvwprintw(win, win_h - 2, win_w - 2, "v");

            wrefresh(win);
        };

        draw();

        while (true) {
            int ch = wgetch(win);

            if (ch == KEY_UP) {
                if (highlight > 0) highlight--;
            }
            else if (ch == KEY_DOWN) {
                if (highlight < total_opts - 1) highlight++;
            }
            else if (ch == KEY_PPAGE) { // Page Up
                highlight -= visible_count;
                if (highlight < 0) highlight = 0;
            }
            else if (ch == KEY_NPAGE) { // Page Down
                highlight += visible_count;
                if (highlight >= total_opts) highlight = total_opts - 1;
            }
            else if (ch == 10 || ch == KEY_ENTER) {  // Enter
                f.value = f.options[highlight];
                break;
            }
            else if (ch == 27) {  // ESC
                break;
            }

            draw();
        }

        delwin(win);
    }

};

int gui() {
    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(1);
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    //if(cols < 80) cols = 80;

    const int h1 = 3, h2 = 3;
    int y1 = 0, y2 = y1 + h1 - 1, y3 = y2 + h2 - 1;

    // ---------- Section 1 ----------
    Section sec1{
        y1, y1 + h1 - 1, {
         {"Gateway:",  "192.168.0.102",  2,  y1 + 1, 17, true, {}},
         {"Path:",     "1,0",            28, y1 + 1, 20, true, {}},
         {"Protocol:", "ab-eip",         55, y1 + 1,  9, true, {"ab-eip","modbus_tcp"}},
         {"Cpu:",      "contrologix",    75, y1 + 1, 13, true, {"plc5","slc","lgxpccc",
                                                        "micrologix800","micrologix",
                                                        "contrologix","omron-njnx"}},
         },
        0
    };

    // ---------- Section 2 ----------
    Section sec2{
        y2, y2 + h2 - 1, {
         {"Type:",    "REAL32",      2,  y2 + 1,  6, true,
             {
                 "BIT","BOOL",
                 "INT8","INT16","INT32","INT64",
                 "UINT8","UINT16","UINT32","UINT64",
                 "REAL32","REAL64","STRING",
             }
         },
         {"Tagname:", "TEST_ARRAY", 16,  y2 + 1, 38, true, {}},
         {"Count:",   "1",          65,  y2 + 1,  3, true, {}},
         {"[Read]",   "",           75,  y2 + 1,  6, false, {}},
         {"[Write]",  "",           85,  y2 + 1,  7, false, {}},
         },
        0
    };

    // ---------- Section 3 ----------
    Section sec3{ y3, rows - 1, {}, 0 };

    vector<Section*> sections = { &sec1, &sec2, &sec3 };

    // ---- RESTORE last session ----
    loadState(sec1, sec2);

    NCursesUI ui(sections);
    ui.run();

    // ---- SAVE on exit ----
    saveState(sec1, sec2);

    endwin();
    return 0;
}
