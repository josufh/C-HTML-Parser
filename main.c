#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum State {
    LookForElement = 0,
    ReadElementName,
    LookForAttributeKey,
    ReadAttributeKey,
    ReadAttributeValue,
    ReadInnerHtml
} ParserState = LookForElement;

typedef struct Attribute {
    char* key;
    char* value;
    struct Attribute* next;
} Attribute;

typedef struct Element {
    char* name;
    struct Attribute* attributes;
    char* inner_html;
} Element;

typedef struct Node {
    struct Element* element;
    struct Node* parent;
    struct Node* child;
    struct Node* next_sibling;
} Node;

Attribute* create_attribute(char* key, char* value) {
    Attribute* new_attribute = (Attribute*)malloc(sizeof(Attribute));
    new_attribute->key = key;
    new_attribute->value = value;
    new_attribute->next = NULL;
    return new_attribute;
}

Element* create_element(char* name) {
    Element* new_element = (Element*)malloc(sizeof(Element));
    new_element->name = name;
    new_element->attributes = NULL;
    new_element->inner_html = NULL;
    return new_element;
}

void add_attribute(Element* element, Attribute* attribute) {
    if (element->attributes == NULL) {
        element->attributes = attribute;
        return;
    }

    Element* temp_element = element;
    while (temp_element->attributes->next != NULL) {
        temp_element->attributes = temp_element->attributes->next;
    }
    temp_element->attributes->next = attribute;
}

Node* create_node(Element* element) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->element = element;
    new_node->parent = NULL;
    new_node->child = NULL;
    new_node->next_sibling = NULL;
    return new_node;
}

void append_child(Node* parent, Node* child) {
    if (parent->child == NULL) {
        parent->child = child;
    } else {
        Node* sibling = parent->child;
        while (sibling->next_sibling != NULL) {
            sibling = sibling->next_sibling;
        }
        sibling->next_sibling = child;
    }
    child->parent = parent;
}

void print_tree(Node* root, int level) {
    if (root == NULL) return;

    for (int i = 0; i < level; i++) {
        printf("\t");
    }
    printf("<%s>", root->element->name);

    printf("  ");
    Attribute* attribute = root->element->attributes;
    while (attribute != NULL) {
        printf("%s=\"%s\"", attribute->key, attribute->value);
        printf("  ");
        attribute = attribute->next;
    }
    printf("\n");

    for (int i = 0; i < level; i++) {
        printf("\t");
    }
    printf("\t%s\n", root->element->inner_html);

    print_tree(root->child, level+1);
    print_tree(root->next_sibling, level);
}

Node* parse_html(const char* html) {
    int html_index = 0;

    int element_name_start;
    int element_name_size;

    int attribute_key_start;
    int attribute_key_size;
    int attribute_value_start;
    int attribute_value_size;

    int inner_html_start;
    int inner_html_size;


    Node* current_node = create_node(create_element("root"));
    
    while (*(html+html_index) != '\0') {
        char* p = (char*)(html+html_index);
        switch (ParserState)
        {
        case LookForElement:
            if (*p == '<') { // Found new element
                if (*(html+html_index+1) != '/') {
                    ParserState = ReadElementName;
                    element_name_start = html_index + 1;
                } else {
                    current_node = current_node->parent;
                }
            }
            break;
        case ReadElementName: 
            if (*p == '>' || *p == ' ') { // Found end of element name
                element_name_size = html_index - element_name_start + 1;
                char* name = (char*)malloc(element_name_size); // Create element name
                strncpy(name, html+element_name_start, element_name_size);
                name[element_name_size - 1] = '\0';
                Element* new_element = create_element(name); // Create new element
                Node* new_node = create_node(new_element); // Create new node
                append_child(current_node, new_node);
                new_node->parent = current_node;
                current_node = new_node;
                if (*p == '>') {
                    ParserState = ReadInnerHtml;
                    inner_html_start = html_index + 1;
                } else if (*p == ' ') {
                    ParserState = LookForAttributeKey;
                }
            }
            break;
        case LookForAttributeKey:
            if (*p == '>') {
                ParserState = ReadInnerHtml;
                inner_html_start = html_index + 1;
            } else if (*p != ' ' && *p != '/') {
                attribute_key_start = html_index;
                ParserState = ReadAttributeKey;
            } else if (*p == '/') {
                char* empty = "";
                current_node->element->inner_html = empty;
                current_node = current_node->parent;
                ParserState = LookForElement;
            }
            break;
        case ReadAttributeKey:
            if (*p == '=') {
                attribute_key_size = html_index - attribute_key_start + 1;
                html_index++; // Skip "
                attribute_value_start = html_index + 1;
                ParserState = ReadAttributeValue;
            }
            break;
        case ReadAttributeValue:
            if (*p == '\"') { // End of attribute value, create attribute
                char* key = (char*)malloc(attribute_key_size);
                strncpy(key, html + attribute_key_start, attribute_key_size);
                key[attribute_key_size - 1] = '\0';

                attribute_value_size = html_index - attribute_value_start + 1;
                char* value = (char*)malloc(attribute_value_size);
                strncpy(value, html + attribute_value_start, attribute_value_size);
                value[attribute_value_size - 1] = '\0';

                Attribute* attribute = create_attribute(key, value);
                add_attribute(current_node->element, attribute);
                free(attribute);
                ParserState = LookForAttributeKey;
            }
            break;
        case ReadInnerHtml:
            if (*p == '<') {
                inner_html_size = html_index - inner_html_start + 1;
                char* inner_html = (char*)malloc(inner_html_size);
                strncpy(inner_html, html + inner_html_start, inner_html_size);
                inner_html[inner_html_size - 1] = '\0';
                current_node->element->inner_html = inner_html;
                if (*(p+1) != '/') {
                    ParserState = ReadElementName;
                    element_name_start = html_index + 1;
                } else {
                    current_node = current_node->parent;
                    ParserState = LookForElement;
                }
            }
            break;
        default:
            break;
        }
        html_index++;
    }

    return current_node;
}

const char* read_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Failed to open file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        perror("Failed to allocate memory");
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        perror("Failed to read file");
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[file_size] = '\0';

    fclose(file);
    return buffer;
}

int main() {
    
    const char* filename = "index.html";
    const char* html_content = read_file(filename);

    printf("Start parsing...\n\n");
    Node* root = parse_html(html_content);
    printf("Printing tree... from %s\n\n", root->element->name);
    print_tree(root->child, 0);

    return 0;
}