#include "ksx.h"

/*一个简单的xml解析器（假设用户都是聪明人）
*/
Queue hash[256]; //用hash算法来查找id
struct XMLNode *getRoot();
struct XMLNode *getChildren(struct XMLNode *);
char *getValue(struct XMLNode *node);

void travelAll(struct XMLNode *node);
char *genNameSpace();
void skipSpace();
void test();
static int isDigit(char c);
XMLNodeP getElementsById(char *id);
int gethash(char *);
static int isLetter(char c);

static int isSign(char c);
static void addID(XMLNodeP, char *);
#define POOLSIZE 1024 * 1024 //文件最大大小
int lineno = 1; //当前行
int column = 0; //当前列
char *src;
char *limit;

enum
{
    ELEMENT = 1,
    ELEMENTEND,
    ID
};
int token;
struct XMLNode *parent = NULL;
struct XMLNode *current = NULL;

struct XMLNode *root = NULL;
struct XMLprolog *prolog = NULL;

void match(char c)
{
    if (*src == c)
        ++src;
    else
    {
        printf("expect '%c' at line %d\n", c, lineno);
        //exit(-1);
    }
}

void nextToken()
{
    int size;
    skipSpace();
    if(src >= limit)
        return;

    char *base;
    char *namespacebase;
    char *namespace;
    //处理cdata
    if (*src == '<' && ((limit - src > 9) ? strncmp(src, "<![CDATA[", 9) : 1))
    {
        src++;

        //处理xml proglog
        if (*src == '?')
        {
            if (*(src + 1) == 'x' && *(src + 2) == 'm' && *(src + 3) == 'l')
            {
                src += 4;
                prolog = malloc(sizeof(struct XMLprolog));
                memset(prolog, 0, sizeof(struct XMLprolog));
            }
            else
            {
                printf("expect xml!!!\n");
                exit(-1);
            }

            skipSpace();

            //获得version
            if (!strncmp(src, "version", 7))
            {
                src += 7;
                match('=');
                match('\"');

                base = src;
                while ((*src >= '0' && *src <= '9') || (*src == '.'))
                {
                    ++src;
                }

                //获得版本值
                size = src - base;
                char *attributeValue = malloc(size + 1);
                strncpy(attributeValue, base, size);
                attributeValue[size] = '\0';
                if (prolog != NULL)
                {
                    prolog->version = attributeValue;
                }
                // printf("version is %s\n", attributeValue);
                match('\"');
            }
            else
            {
                printf("缺少version");
                exit(-1);
            }
            //处理encoding,默认utf-8
            skipSpace();
            if (!strncmp(src, "encoding", 8))
            {
                src += 8;
                match('=');
                match('\"');

                base = src;
                while ((*src >= '0' && *src <= '9') || (*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src == '-'))
                {
                    ++src;
                }

                //处理encoding的值
                size = src - base;
                char *attributeValue = malloc(size + 1);
                strncpy(attributeValue, base, size);
                attributeValue[size] = '\0';
                //printf("encoding is %s\n", attributeValue);
                prolog->encoding = attributeValue;
                match('\"');
            }

            skipSpace();
            //处理standlone
            if (!strncmp(src, "standalone ", 10))
            {
                src += 10;
                match('=');
                match('\"');

                base = src;
                while ((*src >= '0' && *src <= '9') || (*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src == '-'))
                {
                    ++src;
                }

                //处理standlone的值
                size = src - base;
                char *attributeValue = malloc(size + 1);
                strncpy(attributeValue, base, size);
                attributeValue[size] = '\0';
                //printf("standalone is %s\n", attributeValue);
                prolog->standlone = attributeValue;
                match('\"');
            }
            skipSpace();
            match('?');
            match('>');

            //xml proglog 必须出现在第一行，否则为错误
            if (lineno != 1)
            {
                printf("xml proglog 必须出现在第一行");
                exit(-1);
            }
        }
        //处理注释
        else if (*src == '!' && *(src + 1) == '-' && *(src + 2) == '-')
        {
            int commentEnd = 0;
            src += 3;
            while (1 && *src)
            {
                if (*src == '\n')
                    lineno++;
                if (*src == '-' && *(src + 1) == '-' && *(src + 2) == '>')
                {
                    commentEnd = 1;
                    break;
                }

                src++;
            }
            if (commentEnd != 1)
            {
                printf("注释没有正确关闭 在line %d\n", lineno);
                exit(-1);
            }
            src += 3;
        }

        //处理元素的结束
        else if (*src == '/')
        {
            ++src;
            base = src;
            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_') || (*src == ':'))
            {
                //处理命名空间
                if (*src == ':')
                {
                    size = src - base;
                    namespace = malloc(size + 1);
                    strncpy(namespace, base, size);
                    namespace[size] = '\0';
                    base = src + 1;
                }
                ++src;
            }
            if (*src == '>')
            {
                //处理元素结束
                size = src - base;
                char *name = malloc(size + 1);
                strncpy(name, base, size);
                name[size] = '\0';
                //printf("element end is %s\n", end->name);
                src++;
                token = ELEMENTEND;

                //检查是否成对匹配
                if (!strcmp(parent->name, name))
                {
                    parent = parent->parent;
                }
                else
                {
                    printf("%s -- %s mismatch element name at line %d\n", parent->name, name, lineno);
                }
                return;
            }
            else
            {
                printf("expect > at line %d\n", lineno);
                exit(-1);
            }
        }
        //处理元素的开始
        else
        {
            //获取元素名
            base = src;
            if (isLetter(*src) || (*src == '_'))
            {
                src++;
            }
            else
            {
                printf("invalid element name  -- %c !!! at line %d\n", *src, lineno);
                exit(-1);
            }
            while (isLetter(*src) || isDigit(*src) || isSign(*src))
            {
                if (*src == ':')
                {
                    size = src - base; //命名空间
                    namespace = malloc(size + 1);
                    strncpy(namespace, base, size);
                    namespace[size] = '\0';
                    base = src + 1;
                    //printf("====================namespace is %s\n", namespace);
                }
                ++src;
            }
            //将元素加入数据结构中
            if (root == NULL)
            {
                root = malloc(sizeof(struct XMLNode));
                root->type = ELEMENT;
                root->next = NULL;
                root->prev = NULL;
                root->parent = NULL;
                root->children = NULL;
                root->childrenCount = 0;
                root->attributeCount = 0;
                root->attributes = NULL;
                size = src - base;
                root->name = malloc(size + 1);
                strncpy(root->name, base, size);
                root->name[size] = '\0';
                parent = root;
            }
            else
            {
                struct XMLNode *node = malloc(sizeof(struct XMLNode));
                node->type = ELEMENT;
                node->parent = NULL;
                node->next = NULL;
                node->childrenCount = 0;
                node->attributeCount = 0;
                node->attributes = NULL;
                node->prev = node;
                node->children = NULL;
                size = src - base;
                node->name = malloc(size + 1);
                strncpy(node->name, base, size);
                node->name[size] = '\0';

                //xml必须有一个根元素
                if (parent == NULL)
                {
                    printf("xml必须有一个根元素");
                    exit(-1);
                }
                node->parent = parent;
                if (parent->children != NULL)
                {
                    struct XMLNode *tail = parent->children->prev;
                    node->next = tail->next;
                    tail->next = node;
                    node->prev = tail;
                    parent->children->prev = node;
                }
                else
                {
                    parent->children = node;
                }
                (parent != NULL) ? parent->childrenCount++ : 0;
                //parent 设置为当前节点
                parent = node;
                current = node;
            }

            while (*src != '>')
            {
                while (*src == ' ' || *src == '\n' || *src == '\t')
                {
                    if (*src == '\n')
                        lineno++;
                    ++src;
                }
                //自己结束的元素
                if (*src == '/' && *(src + 1) == '>')
                {
                    //将parent设置为上一级
                    parent = parent->parent;
                    src += 2;
                    return;
                }
                while (*src == ' ' || *src == '\n' || *src == '\t')
                {
                    if (*src == '\n')
                        lineno++;
                    ++src;
                }
                //处理属性
                base = src;
                if (isLetter(*src) || (*src == '_')){
                    src++;
                }else{
                    exit(-1);
                }
                while (isLetter(*src) || isDigit(*src) || isSign(*src))
                {
                    if (*src == ':')
                    {
                        //命名空间建立
                        size = src - base;
                        namespace = malloc(size + 1);
                        strncpy(namespace, base, size);
                        namespace[size] = '\0';
                        if (strcmp(namespace, "xmlns"))
                        {
                            printf("expect 'xmlns' \n, not '%s'\n", namespace);
                        }
                        base = src + 1;
                    }
                    ++src;
                }
                //获取属性名
                size = src - base;
                char *attributeName = NULL;
                if (size > 0)
                {
                    attributeName = malloc(size + 1);
                    strncpy(attributeName, base, size);
                    attributeName[size] = '\0';
                }
                else
                {
                    //隐式命名空间
                    attributeName = genNameSpace();
                }

                match('=');
                match('\"');

                base = src;
                while (*src && *src != '\"')
                {
                    ++src;
                }

                //获得属性值
                size = src - base;
                char *attributeValue = malloc(size + 1);
                strncpy(attributeValue, base, size);
                attributeValue[size] = '\0';
                //printf("attribute value is %s\n", attributeValue);
                match('\"');

                struct XMLattributeNode *attr = malloc(sizeof(struct XMLattributeNode));
                attr->key = attributeName;
                attr->value = attributeValue;
                attr->next = NULL;
                attr->prev = attr;

                //设置元素的属性
                if (parent->attributes != NULL)
                {
                    struct XMLattributeNode *tail = parent->attributes->prev;
                    attr->next = tail->next;
                    tail->next = attr;
                    attr->prev = tail;
                    parent->attributes->prev = attr;
                }
                else
                {
                    parent->attributes = attr;
                }
                parent->attributeCount++;
                if(!strcmp(attr->key, "id")){
                    addID(parent, attr->value);
                }
            }
            match('>'); //目的是推进src
            return;
        }
    }

    //处理元素的值
    else
    {
        base = src;
        char *intermedia = NULL; //处理转义字符的中间字符串
        printf("limit - src is %d\n", limit - src);
        while ((*src != '<') || ( (limit - src > 9) ? !strncmp(src, "<![CDATA[", 9) : 0))
        {

            if (!strncmp(src, "<![CDATA[", 9))
            {

                //处理CDATA之前的值
                size = src - base;
                char *prevalue = malloc(size + 1);
                strncpy(prevalue, base, size);
                prevalue[size] = '\0';

                if(intermedia == NULL){
                    intermedia = prevalue;
                }else{
                    //拼接起来
                    char *tmp = intermedia;
                    size = size + strlen(intermedia);
                    intermedia = malloc(size + 1);
                    strcpy(intermedia, tmp);
                    strcat(intermedia, prevalue);
                    intermedia[size] = '\0';
                    free(tmp);
                }
                src += 9;
                base = src;
                while (*src != '\0' && *src != ']' || *(src + 1) != ']' || *(src + 2) != '>')
                {
                    if (*src == '\n')
                        lineno++;
                    src++;
                }

                //构造元素的值，cdata
                size = src - base;
                char *value = malloc(size + 1);
                strncpy(value, base, size);
                value[size] = '\0';
                src += 3;
                base = src;
               
                if(intermedia == NULL){
                    intermedia = value;
                }
                else{
                    char *tmp = intermedia;
                    size = size + strlen(intermedia);
                    intermedia = malloc(size + 1);
                    strcpy(intermedia, tmp);
                    strcat(intermedia, value);
                    intermedia[size] = '\0';
                    free(tmp);
                }
                continue;
            }

            //处理转义字符
            if (*src == '&')
            {
                //处理第一个转义字符
                if (intermedia == NULL)
                {
                    size = src - base + 1;
                    intermedia = malloc(size + 1);
                    strncpy(intermedia, base, size);
                    intermedia[size] = '\0';
                }
                //处理多个转义字符，较麻烦
                else
                {
                    char *tmp = intermedia;
                    size = src - base + 1 + strlen(intermedia);
                    int ssize = src - base + 1;
                    intermedia = malloc(size + 1);
                    strcpy(intermedia, tmp);
                    strncat(intermedia, base, ssize);
                    intermedia[size] = '\0';
                }

                if (*(src + 1) == 'l' && *(src + 2) == 't' && *(src + 3) == ';')
                {
                    intermedia[size - 1] = '<';
                    src += 3;
                }
                else if (*(src + 1) == 'g' && *(src + 2) == 't' && *(src + 3) == ';')
                {
                    intermedia[size - 1] = '>';
                    src += 3;
                }
                else if (*(src + 1) == 'a' && *(src + 2) == 'm' && *(src + 3) == 'p' && *(src + 4) == ';')
                {
                    intermedia[size - 1] = '&';
                    src += 4;
                }
                else if (*(src + 1) == 'a' && *(src + 2) == 'p' && *(src + 3) == 'o' && *(src + 4) == 's' && *(src + 5) == ';')
                {
                    intermedia[size - 1] = '\'';
                    src += 5;
                }
                else if (*(src + 1) == 'q' && *(src + 2) == 'u' && *(src + 3) == 'o' && *(src + 4) == 't' && *(src + 5) == ';')
                {
                    intermedia[size - 1] = '\"';
                    src += 5;
                }
                else
                {
                    printf("& is not occur here at line %d\n", lineno);
                }
                base = src + 1;
            }
            ++src;
        }
        size = src - base;
        char *value = malloc(size + 1);
        strncpy(value, base, size);
        value[size] = '\0';
        if (intermedia != NULL)
        {
            char *tmp = value;
            size = size + strlen(intermedia);
            value = malloc(size + 1);
            strcpy(value, intermedia);
            strcat(value, tmp);
        }
        printf("element value is %s\n", value);
        parent->value = value;
        return;
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("usage program file\n");
        exit(-1);
    }
    char c;
    FILE *fd = fopen(*++argv, "r");

    if (fd == NULL)
    {
        printf("can not open %s\n", *argv[1]);
    }
    int i;

    if ((src = malloc(POOLSIZE)) == NULL)
    {
        printf("malloc error");
    }
    memset(src, 0, POOLSIZE);
    if ((i = fread(src, sizeof c, POOLSIZE - 1, fd)) < 0)
    {
        printf("%d read error!!!\n", i);
    }
    printf("read size is %d\n", i);
    src[i] = 0;
    limit = src + i;

    while (*src)
    {
        nextToken();
    }

    close(fd);

    printf("get id : %s value is %s\n", "xianyu", getElementsById("xianyu")->name);
    return 0;
}

void test()
{
    int length = 0;
    struct XMLNode **result = getElementsByTagName("note", &length);
    int index;
    printf("length is %d\n", length);
    for (index = 0; index < length; index++)
    {
        printf("============ %s\n", getValue(result[index]));
    }

    struct XMLNode **resultc = getChildrenNodes(root, &length);
    if (resultc == NULL)
        exit(-1);
    printf("length is %d\n", length);
    for (index = 0; index < length; index++)
    {
        printf("============ %s\n", getValue(resultc[index]));
        int l = 0;
        struct XMLattributeNode **resultcc = getAttributeNodes(resultc[index], &l);
        if (resultc == NULL)
            exit(-1);
        printf("length is %d\n", l);
        int ii;
        for (ii = 0; ii < l; ii++)
        {
            printf("============ %s\n", (resultcc[ii])->value);
        }
    }

    XMLNodeP node = root->children;
    printf("node parent is %s\n", getParent(node)->name);
    printf("node first child is %s\n", firstChild(node)->name);
    printf("node last child is %s\n", lastChild(node)->name);
    printf("node next sibling is %s\n", nextSibling(node)->name);
    printf("node prev sibiling is %s\n", previousSibling(node)->name);
    char *value = getAttributeValue(node, "index");
    printf("node attr is %s\n", (value == NULL) ? "null" : value);

    travelAll(root);
}

struct XMLNode *getRoot()
{
    return root;
}

struct XMLNode *getChildren(struct XMLNode *parent)
{
    return parent->children;
}

char *getValue(struct XMLNode *node)
{
    return node->value;
}
/*
example:
    int length = 0;
    struct XMLNode** result = getElementsByTagName(name, &lenght);
*/
struct XMLNode **getElementsByTagName(char *name, int *length)
{
    if (name == NULL || root == NULL)
        return;
    Queue queue = Queue_init();
    Queue result = Queue_init();
    enqueue(root, queue);
    while (!isEmpty(queue))
    {
        Node x = dequeue(queue);
        struct XMLNode *xmlnode = x->key;
        while (xmlnode != NULL)
        {
            if (!strcmp(xmlnode->name, name))
            {
                enqueue(xmlnode, result);
            }
            enqueue(xmlnode->children, queue);
            xmlnode = xmlnode->next;
        }
    }
    printf("result size is %d\n", result->size);
    *length = result->size;
    struct XMLNode **resultArray = malloc(sizeof(result->size * sizeof(struct XMLNode)));
    int i = 0;
    while (!isEmpty(result))
    {
        resultArray[i++] = dequeue(result)->key;
    }

    return resultArray;
}

// struct XMLNode* _getElementByName(char *name, struct XMLNode* x, struct XMLNode** result, int index){
//     if(x == NULL) return;
//     if(!strcmp(x->name, name)){
//         result[index++] = x;

//         _getElementByName(name, x->children, index);
//         _getElementByName(name, x->next, index);
//     }
// }

char *genNameSpace()
{
    static int label = 1;
    static char *namebase = "namespace";
    int size = strlen(namebase + 1);
    char *result = malloc(size + 1);
    strcpy(result, namebase);
    result[size - 1] = (char)(label++ - '0');
    result[size] = '\0';
    return result;
}

void travelAll(struct XMLNode *node)
{
    if (node == NULL)
        return;
    while (node != NULL)
    {
        printf("element name is %s and this value is %s\n", node->name, getValue(node));
        struct XMLattributeNode *attr = node->attributes;
        while (attr != NULL)
        {
            printf("%s : %s\n", attr->key, attr->value);
            attr = attr->next;
        }
        travelAll(node->children);
        node = node->next;
    }
}

struct XMLNode **getChildrenNodes(struct XMLNode *x, int *length)
{
    if (x == NULL)
        return;
    *length = x->childrenCount;
    struct XMLNode **result = malloc((*length) * sizeof(struct XMLNode *));
    struct XMLNode *children = x->children;
    int index = 0;
    while (children != NULL)
    {
        result[index++] = children;
        children = children->next;
    }
    return result;
}

struct XMLattributeNode **getAttributeNodes(struct XMLNode *x, int *length)
{
    if (x == NULL)
        return;
    *length = x->attributeCount;
    struct XMLattributeNode *attribute = x->attributes;
    int index = 0;
    struct XMLattributeNode **result = malloc(*length * sizeof(struct XMLattributeNode *));

    while (attribute != NULL)
    {
        result[index++] = attribute;
        attribute = attribute->next;
    }
    return result;
}

struct XMLNode *getParent(struct XMLNode *x)
{
    assert(x != NULL);
    return x->parent;
}
struct XMLNode *firstChild(struct XMLNode *x)
{
    assert(x != NULL);
    return x->children;
}
struct XMLNode *lastChild(struct XMLNode *x)
{
    assert(x != NULL && x->children != NULL);
    return x->children->prev;
}
struct XMLNode *nextSibling(struct XMLNode *x)
{
    assert(x != NULL);
    return x->next;
}
struct XMLNode *previousSibling(struct XMLNode *x)
{
    assert(x != NULL);
    return x->prev;
}

char *getAttributeValue(struct XMLNode *x, char *name)
{
    assert(x != NULL);
    struct XMLattributeNode *attr = x->attributes;
    while (attr != NULL)
    {
        if (!strcmp(attr->key, name))
        {
            return attr->value;
        }
        attr = attr->next;
    }
    return NULL;
}

static int isDigit(char c)
{
    return c >= '0' && c <= '9';
}

static int isLetter(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int isSign(char c)
{
    char signs[] = {'~', '!', '@', '#', '%', '^', '*', '(', ')', '?', ':', ';', '"', ',', '.', '/', '-', '_', '+', '|', ':'};
    int length = sizeof(signs) / sizeof(signs[0]);
    int i;
    for (i = 0; i < length; i++)
    {
        if (c == signs[i])
            return 1;
    }
    return 0;
}

void skipSpace()
{
    while (*src == ' ' || *src == '\n' || *src == '\t' || *src == '\r')
    {
        if (*src == '\n')
            lineno++;
        ++src;
    }
}

//添加id元素
static void addID(XMLNodeP x, char * id){
    x->id = id;
    int hashvalue = gethash(id);
    if(hash[hashvalue] == NULL){
        hash[hashvalue] = Queue_init();
    }

    enqueue(x, hash[hashvalue]);
}

XMLNodeP getElementsById(char *id){
     int hashvalue = gethash(id);
    if(hash[hashvalue] == NULL){
        return NULL;
    }
    Node node = hash[hashvalue]->root->next;
    while(node != NULL){
        if(!strcmp(node->key->id, id)){
            return node->key;
        }
        node = node->next;
    }
    return NULL;
}

int gethash(char *str){
    char *tmp = str;
    int result = 7;
    while(*tmp != '\0'){
        result = result * 31 + (int)*tmp;
        tmp++;
    }
    return result % 256;
}