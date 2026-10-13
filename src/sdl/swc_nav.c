/* SDL presentation of CODE_14's InflightComputer navigation page.
 * The Mac map uses 32-bit world units and its own layout, not WC1 SetScale. */
#include "wc1.h"
#include "swc.h"

typedef struct SwcNavEntry {
    FixedVector position;
    SDL_Point marker;
    SDL_Point label;
    char text[32];
    int frame;
    int color;
    int objective;
} SwcNavEntry;

typedef struct SwcNavMap {
    SwcNavEntry entries[ACTIVE_MISSION_NAV_POINT_COUNT * 10 + MISSION_OBJECTIVE_COUNT + 1];
    int count;
    int minX;
    int maxX;
    int minZ;
    int maxZ;
} SwcNavMap;

typedef struct SwcNavArt {
    SDL_Texture *texture;
    SDL_Rect bounds;
} SwcNavArt;

/* CODE_14 CheckPoint +0x1c04, with Q8-to-world conversion at its callers. */
static void SwcNavCheckPoint(SwcNavMap *map, FixedVector point)
{
    int x = point.x >> 8;
    int z = point.z >> 8;

    map->minX = SDL_min(map->minX, x);
    map->maxX = SDL_max(map->maxX, x);
    map->minZ = SDL_min(map->minZ, z);
    map->maxZ = SDL_max(map->maxZ, z);
}

/* CODE_14 FindTextPos +0x142c and BuildMap +0x14f8. DATA+0x28a
 * supplies (137,138,41,43,0,0); label rows are 138/17+1 pixels high. */
static void SwcBuildNavMap(SwcNavMap *map, const SwcTextFont *font)
{
    unsigned char rows[17] = {0};
    MissionObjective *objective;
    MissionNavPoint *nav;
    MissionShipRecord *ship;
    SwcNavEntry *entry;
    int index;
    int navIndex;
    int slot;
    int record;
    int object;
    int width;
    int height;
    int centreX;
    int centreZ;
    int scale;
    int row;
    int distance;

    SDL_zero(*map);
    map->minX = map->maxX = (cMissionObjectiveCount > 0 ?
        aMissionObjectives[0].position.x : aShipPosition[0].x) >> 8;
    map->minZ = map->maxZ = (cMissionObjectiveCount > 0 ?
        aMissionObjectives[0].position.z : aShipPosition[0].z) >> 8;
    /* SetScale +0x1c8a includes every objective, even hidden objectives,
     * and refreshes mobile positions before calculating the bounds. */
    SwcNavCheckPoint(map, aShipPosition[0]);
    for (index = 0; index < cMissionObjectiveCount; index++) {
        objective = &aMissionObjectives[index];
        if (mobile_objective((short)index)) {
            object = find_ship_index(objective->index);
            if (object >= 0 && object < 10)
                objective->position = aShipPosition[object];
        }
        SwcNavCheckPoint(map, objective->position);
    }
    for (navIndex = 0; navIndex < ACTIVE_MISSION_NAV_POINT_COUNT; navIndex++) {
        nav = &aMissionNavPoints[navIndex];
        if (nav->type == 0)
            break;
        for (slot = 0; slot < 10; slot++) {
            record = nav->missionShips[slot];
            if (record < 0 || record >= ACTIVE_MISSION_SHIP_COUNT)
                continue;
            ship = &aMissionShips[record];
            if (ship->type != OBJECT_TYPE_ASTEROID_FIELD && ship->type != OBJECT_TYPE_MINE_FIELD)
                continue;
            entry = &map->entries[map->count++];
            AddFixedVectors(&nav->position, &ship->position, &entry->position);
            SwcNavCheckPoint(map, entry->position);
            entry->objective = -1;
            entry->frame = ship->type == OBJECT_TYPE_ASTEROID_FIELD ? 3 : 4;
            entry->color = ship->type == OBJECT_TYPE_ASTEROID_FIELD ? 0xf5 : 0xf6;
            SDL_strlcpy(entry->text, ship->type == OBJECT_TYPE_ASTEROID_FIELD ?
                "Asteroids" : "Mines", sizeof(entry->text));
        }
    }
    for (index = 0; index < cMissionObjectiveCount; index++) {
        objective = &aMissionObjectives[index];
        if (objective->type < 0 || objective->type > 4 || hidden_objective((short)index))
            continue;
        if (mobile_objective((short)index)) {
            if (objective_lost((short)index))
                continue;
            /* BuildMap +0x18ec calls find_ships_sphere (CODE_05 +0x6498),
             * whose active-nav preference and wave fallback match WC1. */
            navIndex = find_ships_sphere(objective->index);
            if (navIndex >= 0 && navIndex < ACTIVE_MISSION_NAV_POINT_COUNT) {
                nav = &aMissionNavPoints[navIndex];
                AddFixedVectors(&nav->position,
                    &aMissionShips[objective->index].position, &objective->position);
            }
        }
        entry = &map->entries[map->count++];
        entry->position = objective->position;
        entry->objective = index;
        entry->frame = objective->type < 2 ? objective->type : 2;
        if (objective->type == 0)
            entry->color = visited((short)index) ? 0xf8 : 0xf9;
        else
            entry->color = objective->type == 1 ? 0xfa : objective->type == 4 ? 0xfb : 0xfc;
        SDL_strlcpy(entry->text, objective->displayName, sizeof(entry->text));
    }
    entry = &map->entries[map->count++];
    entry->position = aShipPosition[0];
    entry->frame = 2;
    entry->color = 0xf8;
    entry->objective = -1;
    SDL_strlcpy(entry->text, (const char *)abMissionAuxData, sizeof(entry->text));

    /* SetScale, ScaleX +0x1eaa and ScaleY +0x1f06. Preserve the Mac's
     * asymmetric 10,000-unit margin and its integer division order. */
    width = 10000 + map->maxX - map->minX;
    height = 10000 + map->maxZ - map->minZ;
    centreX = map->minX + width / 2;
    centreZ = map->maxZ - height / 2;
    scale = SDL_max(1, SDL_max(width / 137, height / 138));
    for (index = 0; index < map->count; index++) {
        entry = &map->entries[index];
        entry->marker.x = 41 + 137 / 2 + ((entry->position.x >> 8) - centreX) / scale;
        entry->marker.y = 43 + 138 / 2 - ((entry->position.z >> 8) - centreZ) / scale;
        row = SDL_clamp((entry->marker.y - 43) / 9, 0, 16);
        if (rows[row]) {
            for (distance = 1; distance < 3; distance++) {
                if (row + distance <= 16 && !rows[row + distance]) {
                    row += distance;
                    break;
                }
                if (row - distance >= 0 && !rows[row - distance]) {
                    row -= distance;
                    break;
                }
            }
        }
        rows[row] = 1;
        entry->label.x = SDL_min(entry->marker.x,
            41 + 137 - SdlSwcTextWidth(font, entry->text)) - 1;
        entry->label.y = 43 + row * 9 + 8;
    }
}

/* InflightComputer uses set_new_objective's flight-path index, which is
 * distinct from a marker's objective index. Bound the search if all hide. */
static void SwcSelectNavObjective(int direction, int objective)
{
    int index = cCurrentNavPointIndex;
    int previousIndex = index;
    int count = cMissionObjectiveCount;
    int attempt;

    for (attempt = 0; attempt < count; attempt++) {
        if (objective >= 0)
            index = attempt;
        else
            index = (index + direction + count) % count;
        if (abFlightPath[index] < 0 || abFlightPath[index] >= count ||
            (objective >= 0 && abFlightPath[index] != objective))
            continue;
        if (set_new_objective((short)index))
            return;
    }
    cCurrentNavPointIndex = (signed char)previousIndex;
}

static int SwcLoadNavArt(SDL_Renderer *renderer, const SwcCmf *cmf,
                          const SDL_Color colors[256], int id, int frame, SwcNavArt *art)
{
    SwcBuffer set = {0};
    SwcFrame shape;
    int result = -1;

    if (SwcCMGetChunk(cmf, "CKPT", (uint32_t)id, &set) == 0 &&
        SwcGetFramePtr(&set, (uint32_t)frame, &shape) == 0) {
        art->bounds = (SDL_Rect){shape.x, shape.y, shape.width, shape.height};
        art->texture = SdlCreateSwcTexture(renderer, &set, (uint32_t)frame, colors);
        if (art->texture != NULL)
            result = 0;
    }
    SDL_free(set.data);
    return result;
}

static int SwcDrawNavArt(SDL_Renderer *renderer, const SwcNavArt *art, int x, int y)
{
    SDL_Rect destination = art->bounds;

    destination.x += x;
    destination.y += y;
    return SDL_RenderCopy(renderer, art->texture, NULL, &destination);
}

/* UpdateInflightNavText +0x273a: size 9 Helvetica, 11-pixel newline,
 * white body text, yellow headings and the selected objective's note. */
static int SwcDrawNavText(SDL_Renderer *renderer, const SwcTextFont *font,
                          const SDL_Color colors[256])
{
    const char *text[] = {"ConFed Nav", "Scan", "", "SECTOR:", "XR-231.3",
        "SYSTEM:", (const char *)abSeriesAuxData, "", (const char *)abMissionAuxData, ""};
    SDL_Rect clip = {178, 43, 80, 138};
    int role = aMissionShips[nPlayerMissionShipIndex].missionType;
    int index;
    int result = -1;

    if (role >= 0 && role < 11)
        text[9] = apszShipMissionTypeNames[role];
    if (SDL_RenderSetClipRect(renderer, &clip) != 0)
        return -1;
    for (index = 0; index < (int)SDL_arraysize(text); index++) {
        if (SdlDrawSwcProportionalText(renderer, font, 178, 52 + index * 11,
                colors[index == 3 || index == 5 ? 0xf9 : 1], text[index]) != 0)
            goto done;
    }
    clip = (SDL_Rect){41, 43, 217, 145};
    if (SDL_RenderSetClipRect(renderer, &clip) != 0)
        goto done;
    if (cCurrentObjective >= 0 && cCurrentObjective < cMissionObjectiveCount &&
        SdlDrawSwcProportionalText(renderer, font, 45, 179, colors[1],
            nav_note(cCurrentObjective)) != 0)
        goto done;
    result = 0;
done:
    if (SDL_RenderSetClipRect(renderer, NULL) != 0)
        result = -1;
    return result;
}

/* CODE_14 InflightComputer +0x2dd2, navigation page (0). Simulation stays
 * suspended; SDL owns input, cursor lifetime and redraw pacing. Other
 * computer pages remain with their existing cockpit controls. */
int SdlShowSwcNavigation(SDL_Window *window, SDL_Renderer *renderer, const char *resourceFork)
{
    SwcCmf cmf = {0};
    SwcNavArt art[9] = {0}; /* Frame, map, selected Nav tab, eject guard, five markers. */
    SwcTextFont labelFont = {0};
    SwcTextFont textFont = {0};
    SwcNavMap map;
    SwcNavEntry *entry;
    SDL_Color colors[256];
    SDL_Color markerColors[256];
    SDL_Event event;
    SDL_Rect savedClip;
    SDL_Point click;
    SDL_Rect hit;
    SDL_bool clipped = SDL_RenderIsClipEnabled(renderer);
    SDL_bool grabbed = SDL_GetWindowMouseGrab(window);
    SDL_Scancode closeKey = SDL_SCANCODE_UNKNOWN;
    int savedCursor = SDL_ShowCursor(SDL_QUERY);
    int focused = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    int mouseDown = 0;
    int index;
    int result = SWC_MOVIE_ERROR;
    Uint32 previous = SDL_GetTicks();
    Uint32 current;
    Uint32 elapsed = 0;
    char path[PATH_MAX];
    char failure[256];

    SDL_RenderGetClipRect(renderer, &savedClip);
    if (!SdlResolvePath("CMFs/ComputerInflight.CMF", path, sizeof(path)) ||
        SwcCMOpen(path, &cmf) != 0 || SwcReadPalette(resourceFork, 250, colors) != 0 ||
        SdlCreateSwcTextFont(renderer, &labelFont, 10) != 0 ||
        SdlCreateSwcTextFont(renderer, &textFont, 9) != 0)
        goto done;
    for (index = 0; index < 256; index++)
        markerColors[index] = (SDL_Color){255, 255, 255, 255};
    if (SwcLoadNavArt(renderer, &cmf, colors, 2, 0, &art[0]) != 0 ||
        SwcLoadNavArt(renderer, &cmf, colors, 3, 0, &art[1]) != 0 ||
        SwcLoadNavArt(renderer, &cmf, colors, 4, 0, &art[2]) != 0 ||
        SwcLoadNavArt(renderer, &cmf, colors, 13, 0, &art[3]) != 0)
        goto done;
    for (index = 0; index < 5; index++) {
        if (SwcLoadNavArt(renderer, &cmf, markerColors, 10, index, &art[4 + index]) != 0)
            goto done;
    }
    SwcBuildNavMap(&map, &labelFont);
    if (SDL_RenderSetClipRect(renderer, NULL) != 0)
        goto done;
    SDL_SetWindowMouseGrab(window, SDL_FALSE);
    SDL_ShowCursor(SDL_ENABLE);
    SdlPauseSwcSound(1);
    for (;;) {
        current = SDL_GetTicks();
        if (focused)
            elapsed += SDL_min(current - previous, 250);
        previous = current;
        while (SDL_PollEvent(&event)) {
            if (SdlHandleSwcEvent(window, &event, 0))
                continue;
            if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE)) {
                result = SWC_MOVIE_QUIT;
                goto done;
            }
            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST ||
                    event.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                    focused = 0;
                    mouseDown = 0;
                    closeKey = SDL_SCANCODE_UNKNOWN;
                } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED ||
                           event.window.event == SDL_WINDOWEVENT_RESTORED) {
                    focused = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
                }
            }
            if (!focused)
                continue;
            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                switch (event.key.keysym.sym) {
                case SDLK_m:
                case SDLK_ESCAPE:
                case SDLK_BACKSPACE:
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                case SDLK_SPACE:
                    closeKey = event.key.keysym.scancode;
                    break;
                case SDLK_LEFT:
                case SDLK_UP:
                    SwcSelectNavObjective(-1, -1);
                    break;
                case SDLK_RIGHT:
                case SDLK_DOWN:
                case SDLK_n:
                    SwcSelectNavObjective(1, -1);
                    break;
                default:
                    break;
                }
            }
            if (event.type == SDL_KEYUP && closeKey != SDL_SCANCODE_UNKNOWN &&
                event.key.keysym.scancode == closeKey) {
                result = SWC_MOVIE_FINISHED;
                goto done;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
                mouseDown = 1;
            if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
                click = (SDL_Point){event.button.x, event.button.y};
                hit = (SDL_Rect){262, 65, 35, 20};
                if (mouseDown && SDL_PointInRect(&click, &hit)) {
                    result = SWC_MOVIE_FINISHED;
                    goto done;
                }
                for (index = 0; mouseDown && index < map.count; index++) {
                    entry = &map.entries[index];
                    if (entry->objective < 0)
                        continue;
                    hit = (SDL_Rect){entry->marker.x - 3, entry->marker.y - 3, 15, 15};
                    if (!SDL_PointInRect(&click, &hit)) {
                        hit = (SDL_Rect){entry->label.x, entry->label.y - 9,
                            SdlSwcTextWidth(&labelFont, entry->text), 11};
                    }
                    if (SDL_PointInRect(&click, &hit)) {
                        SwcSelectNavObjective(0, entry->objective);
                        break;
                    }
                }
                mouseDown = 0;
            }
        }
        if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) != 0 ||
            SDL_RenderClear(renderer) != 0 ||
            SwcDrawNavArt(renderer, &art[0], 0, 0) != 0 ||
            SwcDrawNavArt(renderer, &art[1], 41, 43) != 0 ||
            SwcDrawNavArt(renderer, &art[2], 262, 65) != 0 ||
            SwcDrawNavArt(renderer, &art[3], 263, 179) != 0)
            goto done;
        for (index = 0; index < map.count; index++) {
            entry = &map.entries[index];
            if (entry->objective < 0 || entry->objective != cCurrentObjective ||
                (elapsed / 50) % 6 < 3) {
                if (SDL_SetTextureColorMod(art[4 + entry->frame].texture,
                        colors[entry->color].r, colors[entry->color].g, colors[entry->color].b) != 0 ||
                    SwcDrawNavArt(renderer, &art[4 + entry->frame],
                        entry->marker.x, entry->marker.y) != 0)
                    goto done;
            }
        }
        for (index = 0; index < map.count; index++) {
            entry = &map.entries[index];
            if (SdlDrawSwcProportionalText(renderer, &labelFont,
                    entry->label.x, entry->label.y, colors[entry->color], entry->text) != 0)
                goto done;
        }
        if (SwcDrawNavText(renderer, &textFont, colors) != 0)
            goto done;
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
done:
    if (result == SWC_MOVIE_ERROR)
        SDL_strlcpy(failure, SDL_GetError(), sizeof(failure));
    SDL_RenderSetClipRect(renderer, clipped ? &savedClip : NULL);
    focused = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    SDL_SetWindowMouseGrab(window, focused ? grabbed : SDL_FALSE);
    SDL_ShowCursor(focused && savedCursor == SDL_DISABLE ? SDL_DISABLE : SDL_ENABLE);
    SdlPauseSwcSound(!focused);
    SDL_DestroyTexture(labelFont.texture);
    SDL_DestroyTexture(textFont.texture);
    for (index = 0; index < (int)SDL_arraysize(art); index++)
        SDL_DestroyTexture(art[index].texture);
    SwcCMClose(&cmf);
    if (result == SWC_MOVIE_ERROR)
        SDL_SetError("%s", failure);
    return result;
}
