#ifndef LIST_TAGS_H
#define LIST_TAGS_H

#include <string>
#include <vector>


struct TagEntry {
    std::string nextName;
    std::string name;
    std::string parentName;
    uint16_t instance_id;
    uint16_t type;
    uint16_t elem_size;
    uint16_t elem_count;
    uint16_t num_dimensions;
    uint16_t dimensions[3];
};

// struct program_entry_s {
//     struct program_entry_s *next;
//     char *program_name;
// };

// struct tag_entry_s {
//     struct tag_entry_s *next;
//     char *name;
//     struct tag_entry_s *parent;
//     uint16_t instance_id;
//     uint16_t type;
//     uint16_t elem_size;
//     uint16_t elem_count;
//     uint16_t num_dimensions;
//     uint16_t dimensions[3];
// };


// struct udt_field_entry_s {
//     char *name;
//     uint16_t type;
//     uint16_t metadata;
//     uint32_t size;
//     uint32_t offset;
// };


// struct udt_entry_s {
//     char *name;
//     uint16_t id;
//     uint16_t num_fields;
//     uint16_t struct_handle;
//     uint32_t instance_size;
//     struct udt_field_entry_s fields[];
// };

int list_tags(int argc, const char **argv, std::vector<TagEntry> &tagentries);


#endif // LIST_TAGS_H
