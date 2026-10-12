/* SDL replacement for Resource Manager/GetCTable, not recovered game code.
 * Accept an extracted raw resource fork or macOS's ..namedfork/rsrc path. */
#include "swc.h"

#include <string.h>

int SwcReadResource(const char *resourceFork, const char typeName[4], int16_t id,
                    SwcBuffer *resource)
{
    uint8_t *data;
    size_t size;
    SDL_RWops *reader;
    uint32_t dataOffset;
    uint32_t mapOffset;
    uint32_t dataSize;
    uint32_t mapSize;
    uint32_t resourceOffset;
    uint32_t resourceSize;
    uint32_t typeCount;
    uint32_t referenceCount;
    uint32_t type;
    uint32_t reference;
    size_t typeOffset;
    size_t referenceOffset;
    size_t mapEnd;
    size_t position;
    char resourceType[4];
    int16_t resourceId;
    int result;

    memset(resource, 0, sizeof(*resource));
    data = SDL_LoadFile(resourceFork, &size);
    if (data == NULL)
        return -1;
    result = -1;
    reader = NULL;
    if (size < 16 || size > INT32_MAX)
        goto malformed;
    reader = SDL_RWFromConstMem(data, (int)size);
    if (reader == NULL)
        goto done;
    dataOffset = SDL_ReadBE32(reader);
    mapOffset = SDL_ReadBE32(reader);
    dataSize = SDL_ReadBE32(reader);
    mapSize = SDL_ReadBE32(reader);
    if (dataOffset > size || dataSize > size - dataOffset ||
        mapOffset > size || mapSize > size - mapOffset || mapSize < 28)
        goto malformed;
    mapEnd = (size_t)mapOffset + mapSize;
    SDL_RWseek(reader, (Sint64)mapOffset + 24, RW_SEEK_SET);
    typeOffset = SDL_ReadBE16(reader);
    if (typeOffset < 28 || typeOffset > mapSize - 2)
        goto malformed;
    typeOffset += mapOffset;
    SDL_RWseek(reader, (Sint64)typeOffset, RW_SEEK_SET);
    typeCount = SDL_ReadBE16(reader);
    typeCount = typeCount == UINT16_MAX ? 0 : typeCount + 1;
    if (typeCount > (mapEnd - typeOffset - 2) / 8)
        goto malformed;
    for (type = 0; type < typeCount; type++) {
        SDL_RWseek(reader, (Sint64)(typeOffset + 2 + type * 8), RW_SEEK_SET);
        SDL_RWread(reader, resourceType, 1, 4);
        referenceCount = (uint32_t)SDL_ReadBE16(reader) + 1;
        referenceOffset = SDL_ReadBE16(reader);
        if (referenceOffset > mapEnd - typeOffset)
            goto malformed;
        referenceOffset += typeOffset;
        if (referenceCount > (mapEnd - referenceOffset) / 12)
            goto malformed;
        if (memcmp(resourceType, typeName, 4) != 0)
            continue;
        for (reference = 0; reference < referenceCount; reference++) {
            position = referenceOffset + reference * 12;
            SDL_RWseek(reader, (Sint64)position, RW_SEEK_SET);
            resourceId = (int16_t)SDL_ReadBE16(reader);
            if (resourceId != id)
                continue;
            SDL_RWseek(reader, 2, RW_SEEK_CUR);
            resourceOffset = SDL_ReadBE32(reader) & UINT32_C(0x00ffffff);
            if (resourceOffset > dataSize || dataSize - resourceOffset < 4)
                goto malformed;
            SDL_RWseek(reader, (Sint64)dataOffset + resourceOffset, RW_SEEK_SET);
            resourceSize = SDL_ReadBE32(reader);
            if (resourceSize == 0 || resourceSize > dataSize - resourceOffset - 4)
                goto malformed;
            resource->data = SDL_malloc(resourceSize);
            if (resource->data == NULL) {
                SDL_OutOfMemory();
                goto done;
            }
            resource->size = resourceSize;
            memcpy(resource->data, data + dataOffset + resourceOffset + 4, resourceSize);
            result = 0;
            goto done;
        }
    }
    SDL_SetError("Resource %.4s/%d not found in %s", typeName, id, resourceFork);
    goto done;

malformed:
    SDL_SetError("Malformed Mac resource fork: %s", resourceFork);
done:
    if (reader != NULL)
        SDL_RWclose(reader);
    SDL_free(data);
    return result;
}

int SwcReadPalette(const char *resourceFork, int16_t id, SDL_Color colors[256])
{
    SwcBuffer resource = {0};
    SDL_RWops *reader = NULL;
    uint16_t flags;
    uint16_t count;
    uint16_t index;
    uint32_t color;
    int result = -1;

    if (SwcReadResource(resourceFork, "clut", id, &resource) != 0)
        return -1;
    if (resource.size < 8)
        goto malformed;
    reader = SDL_RWFromConstMem(resource.data, (int)resource.size);
    if (reader == NULL)
        goto done;
    SDL_RWseek(reader, 4, RW_SEEK_SET);
    flags = SDL_ReadBE16(reader);
    count = SDL_ReadBE16(reader);
    if (count > 255 || (size_t)(count + 1) * 8 > resource.size - 8)
        goto malformed;
    for (color = 0; color < 256; color++)
        colors[color] = (SDL_Color){0, 0, 0, SDL_ALPHA_OPAQUE};
    for (color = 0; color <= count; color++) {
        index = SDL_ReadBE16(reader);
        if (flags & 0x8000)
            index = (uint16_t)color;
        if (index > 255)
            goto malformed;
        colors[index].r = (uint8_t)(SDL_ReadBE16(reader) >> 8);
        colors[index].g = (uint8_t)(SDL_ReadBE16(reader) >> 8);
        colors[index].b = (uint8_t)(SDL_ReadBE16(reader) >> 8);
    }
    result = 0;
    goto done;
malformed:
    SDL_SetError("Malformed Mac palette clut/%d: %s", id, resourceFork);
done:
    if (reader != NULL)
        SDL_RWclose(reader);
    SDL_free(resource.data);
    return result;
}
