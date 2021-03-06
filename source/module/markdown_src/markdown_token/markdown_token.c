
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "markdown_token.h"
#include "../markdown_parser/markdown_sentence.h"
#include <regex.h>
#include "markdown_headnodes.h"


bool TOC = false;
nodelist list;

void init_token (md_token *token) {
    token->m_size = 0;
    memset(token->str, '\0', sizeof(char) * 1024);
}

bool issymbol (char c) {
    switch (c) {
        case '-' :
        case '*':
        case '~':
        case '`':
        case '#':
        case '!':
        case '^':
        case '<':
            /*case '>':*/
        case '@'://表示高亮
        case '_':
            return true;
        default:
            return false;
    }
}

bool is_space (char c) {
    return c == ' ' || c == '\t' || c == '\v' || c == '\f';
}

void skip_space (char *file_path, char *tmp_file) {
    char pre = '\0';
    FILE *input_md, *tmp_md;
    char c;
    input_md = fopen(file_path, "r");
    if (!input_md) {
        printf("请输入有效的md文件地址\n");
        fflush(stdout);
        exit(1);
    }
    tmp_md = fopen(tmp_file, "w");
    if (!tmp_md) {
        printf("请输入有效的缓冲文件地址\n");
        fflush(stdout);
        exit(1);
    }

    while (fread(&c, 1, 1, input_md)) {
        if (((pre != '#' && pre != '*' && pre != '-') && ((is_space(pre) && is_space(c)))))
            continue;
        else {

            if ((int) c == 10) {
                c = '\n';
                fwrite(&c, 1, 1, tmp_md);
            } else {
                fwrite(&c, 1, 1, tmp_md);
            }
        }
        pre = c;
    }
    fclose(input_md);
    fclose(tmp_md);
}


void read_token (md_token *mt, FILE *f) {
    char c = '\0';
    while (true) {
        if (!fread(&c, 1, 1, f)) {
            break;
        }
        if (issymbol(c)) {
            break;
        }
        mt->str[mt->m_size++] = c;
    }
    if (issymbol(c)) {
        /**后退一个位置*/
        fseek(f, -1, SEEK_CUR);
    }
}

void produce_token (md_token *mt, FILE *f) {
    char fc;//字串的第一个字符
    if (fread(&fc, 1, 1, f)) {
        size_t ret;
        switch (fc) {
            case '_': {
                /**下划线功能的实现*/
                read_token(mt, f);
                if (fread(&fc, sizeof(char), 1, f)) {
                    if (fc == '_') {
                        mt->type = MD_UNDER;
                    } else {
                        fseek(f, -1, SEEK_CUR);
                        mt->type = MD_PLAIN;
                        memmove(mt->str + 1, mt->str, strlen(mt->str));
                        mt->str[0] = '_';
                        mt->m_size++;
                    }
                } else {
                    mt->type = MD_PLAIN;
                    memmove(mt->str + 1, mt->str, strlen(mt->str));
                    mt->str[0] = '_';
                    mt->m_size++;
                }
                break;
            }
                /* case '}': {
                     if (fread(&fc, sizeof(char), 1, f)) {
                         if (fc == '>') {
                             read_token(mt, f);
                             mt->type = MD_RIGHT;
                         } else {
                             mt->str[mt->m_size++] = '>';
                             mt->type = MD_PLAIN;
                             fseek(f, -1, SEEK_CUR);
                         }
                     } else {
                         mt->str[mt->m_size++] = '>';
                         mt->type = MD_PLAIN;
                     }
                     break;
                 }*/
            case '@': {
                read_token(mt, f);
                if (fread(&fc, sizeof(char), 1, f)) {
                    if (fc == '@') {
                        mt->type = MD_HIGHLIGHT;
                    } else {
                        fseek(f, -1, SEEK_CUR);
                        mt->type = MD_PLAIN;
                        memmove(mt->str + 1, mt->str, strlen(mt->str));
                        mt->str[0] = '@';
                        mt->m_size++;
                    }
                } else {
                    mt->type = MD_PLAIN;
                    memmove(mt->str + 1, mt->str, strlen(mt->str));
                    mt->str[0] = '@';
                    mt->m_size++;
                }
                break;
            }
            case '<': {
                if (fread(&fc, sizeof(char), 1, f)) {
                    if (fc == 'x') {
                        mt->type = MD_CHECKBOX;
                        mt->str[mt->m_size++] = 'x';
                        if (!fread(&fc, sizeof(char), 1, f)) {
                            mt->m_size = 2;
                            mt->str[0] = '<';
                            mt->str[1] = 'x';
                            mt->type = MD_PLAIN;
                        } else {
                            //第三个字符读取成功
                            if (fc == '>') {
                                break;
                            } else {
                                fseek(f, -1, SEEK_CUR);
                                mt->m_size = 2;
                                mt->str[0] = '<';
                                mt->str[1] = 'x';
                                mt->type = MD_PLAIN;
                            }
                        }
                    } else if (fc == 'y') {
                        mt->type = MD_CHECKBOX;
                        mt->str[mt->m_size++] = 'y';
                        if (!fread(&fc, sizeof(char), 1, f)) {
                            mt->m_size = 2;
                            mt->str[0] = '<';
                            mt->str[1] = 'y';
                            mt->type = MD_PLAIN;
                        } else {
                            //第三个字符读取成功
                            if (fc == '>') {
                                break;
                            } else {
                                fseek(f, -1, SEEK_CUR);
                                mt->m_size = 2;
                                mt->str[0] = '<';
                                mt->str[1] = 'y';
                                mt->type = MD_PLAIN;
                            }
                        }
                    } else {
                        fseek(f, -1, SEEK_CUR);
                        mt->type = MD_PLAIN;
                        mt->str[mt->m_size++] = '<';

                    }
                } else {
                    mt->type = MD_PLAIN;
                    mt->str[mt->m_size++] = '<';
                }
                break;
            }
            case '!': {
                while (true) {
                    char c;
                    if (!fread(&c, 1, 1, f)) {
                        break;
                    } else {
                        mt->str[mt->m_size++] = c;
                        if (c == ')') {
                            break;
                        }
                    }
                }

                int res;
                int len;
                char result[BUFSIZ];
                const char *pattern = "\\[.*\\]\\((.+)\\)";
                regex_t preg;
                regmatch_t pmatch[10];
                if ((res = regcomp(&preg, pattern, REG_EXTENDED)) != 0) {
                    return;
                }
                res = regexec(&preg, mt->str, 10, pmatch, REG_NOTBOL);
                if (res == REG_NOMATCH) {
                    mt->type = MD_PLAIN;
                    return;
                }
                for (regoff_t i = 1; pmatch[i].rm_so != -1; i++) {
                    len = pmatch[i].rm_eo - pmatch[i].rm_so;
                    memcpy(result, mt->str + pmatch[i].rm_so, len);
                    result[len] = 0;
                }
                regfree(&preg);
                memset(mt->str, '\0', strlen(mt->str));
                strcpy(mt->str, result);
                mt->type = MD_IMG;
                mt->m_size = (int) strlen(mt->str);
                break;
            }
            case '^' : {
                int right_tag = 0;
                char c = '\0';
                while (true) {
                    if (!fread(&c, 1, 1, f)) {
                        break;
                    }
                    if (c == ']') {
                        ++right_tag;
                        if (right_tag == 2) {
                            mt->str[mt->m_size++] = ']';

                            break;
                        }
                    }
                    mt->str[mt->m_size++] = c;
                }

                int res;
                const char *pattern = "\\[.*\\]\\[(.+)\\]";
                regex_t preg;
                regmatch_t pmatch[10];
                if ((res = regcomp(&preg, pattern, REG_EXTENDED)) != 0) {
                    init_token(mt);
                    return;
                }
                res = regexec(&preg, mt->str, 10, pmatch, REG_NOTBOL);
                if (res == REG_NOMATCH) {
                    mt->type = MD_PLAIN;
                } else {
                    mt->type = MD_LINK;
                }
                regfree(&preg);
                break;
            }
            case '-' : {
                char buf[12] = {0};
                if ((ret = fread(buf, sizeof(char), 2, f)) != 0) {
                    if (!strcmp(buf, "--")) {
                        mt->type = MD_LINE;
                        mt->m_size = 1;
                        return;
                    }
                } else {
                    mt->type = MD_UD_LIST;
                    mt->str[mt->m_size++] = '-';
                    return;
                }
                fseek(f, -ret, SEEK_CUR);
                if ((ret = fread(&fc, sizeof(char), 1, f)) != 0) {
                    if (fc == ' ') {
                        mt->type = MD_UD_LIST;
                        read_token(mt, f);
                    } else if (fc == '-') {
                        mt->type = MD_PLAIN;
                        mt->str[mt->m_size++] = '-';
                        mt->str[mt->m_size++] = '-';
                    } else {
                        mt->type = MD_PLAIN;
                        //回退一个位置
                        fseek(f, -1, SEEK_CUR);
                        mt->str[mt->m_size++] = '-';
                        read_token(mt, f);
                    }
                }
                break;
            }
            case '*': {
                if (fread(&fc, 1, 1, f)) {
                    if (fc == ' ') {
                        /*unorder list*/
                        read_token(mt, f);
                        mt->type = MD_UD_LIST;
                        return;
                    } else if (fc == '*') {
                        char c = '\0';
                        while (true) {
                            if (!fread(&c, 1, 1, f)) {
                                break;
                            }
                            if (c == '*') {
                                fseek(f, -1, SEEK_CUR);
                                break;
                            }
                            mt->str[mt->m_size++] = c;
                        }
                        if (fread(&fc, 1, 1, f)) {
                            if (fc == '*') {
                                if (fread(&fc, 1, 1, f)) {
                                    if (fc == '*') {
                                        mt->type = MD_IMPT;
                                    } else {
                                        memmove(mt->str + 2, mt->str, strlen(mt->str));
                                        mt->str[0] = mt->str[1] = '*';
                                        mt->m_size += 2;
                                        mt->type = MD_PLAIN;
                                        fseek(f, -2, SEEK_CUR);
                                    }
                                } else {
                                    fseek(f, -1, SEEK_CUR);

                                    memmove(mt->str + 2, mt->str, strlen(mt->str));
                                    mt->str[0] = mt->str[1] = '*';
                                    mt->m_size += 2;
                                    mt->type = MD_PLAIN;
                                }
                            } else {
                                memmove(mt->str + 2, mt->str, strlen(mt->str));
                                mt->str[0] = mt->str[1] = '*';
                                mt->m_size += 2;
                                mt->type = MD_PLAIN;
                                fseek(f, -1, SEEK_CUR);
                            }
                        } else {//
                            memmove(mt->str + 2, mt->str, strlen(mt->str));
                            mt->str[0] = mt->str[1] = '*';
                            mt->m_size += 2;
                            mt->type = MD_PLAIN;
                        }

                    } else {
                        fseek(f, -1, SEEK_CUR);
                        read_token(mt, f);
                        if (!strcmp("", mt->str)) {
                            mt->str[0] = '*';
                            mt->m_size = 1;
                            mt->type = MD_PLAIN;
                            return;
                        } else {
                            if (fread(&fc, 1, 1, f)) {
                                if (fc == '*') {
                                    mt->type = MD_ILY;
                                } else {
                                    mt->type = MD_PLAIN;
                                    mt->str[mt->m_size++] = '*';
                                    fseek(f, -1, SEEK_CUR);
                                }
                            } else {
                                memmove(mt->str + 1, mt->str, strlen(mt->str));
                                mt->str[0] = '*';
                                mt->m_size += 1;
                                mt->type = MD_PLAIN;
                            }
                        }

                    }
                } else {
                    mt->str[0] = '*';
                    mt->m_size += 1;
                    mt->type = MD_PLAIN;
                    return;
                }
                break;
            }
            case '~': {
                if (fread(&fc, 1, 1, f)) {
                    if (fc == '~') {
                        mt->type = MD_DEL;
                        read_token(mt, f);
                        if (!fread(&fc, 1, 1, f)) {
                            mt->str[0] = mt->str[1] = '~';
                            mt->m_size = 2;
                            mt->type = MD_PLAIN;
                            break;
                        }
                        if (fc == '~') {
                            ret = fread(&fc, 1, 1, f);
                            if (!ret) {
                                mt->type = MD_PLAIN;
                                memmove(mt->str + 2, mt->str, mt->m_size);
                                mt->str[0] = mt->str[1] = '~';
                                return;
                            } else {
                                if (fc == '~') {
                                    return;
                                } else {
                                    mt->type = MD_PLAIN;
                                    memmove(mt->str + 2, mt->str, mt->m_size);
                                    mt->str[0] = mt->str[1] = '~';
                                    fseek(f, -2, SEEK_CUR);
                                }
                            }
                        } else {
                            mt->type = MD_PLAIN;
                            memmove(mt->str + 2, mt->str, mt->m_size);
                            mt->str[0] = mt->str[1] = '~';
                            fseek(f, -1, SEEK_CUR);
                        }
                    } else {
                        mt->type = MD_PLAIN;
                        mt->str[mt->m_size++] = '~';
                        fseek(f, -1, SEEK_CUR);
                        return;
                    }
                } else {
                    mt->str[mt->m_size++] = '~';
                }
                break;

            }
            case '`': {
                fseek(f, -1, SEEK_CUR);
                int p_size = 0;
                while (fread(&fc, 1, 1, f)) {
                    if (fc == '`') {
                        p_size++;
                    } else {
                        fseek(f, -1, SEEK_CUR);
                        break;
                    }
                }
                if (p_size == 3) {
                    read_token(mt, f);
                    p_size = 0;
                    while (fread(&fc, 1, 1, f)) {
                        if (fc == '`') {
                            p_size++;
                        } else {
                            fseek(f, -1, SEEK_CUR);
                            break;
                        }
                    }
                    if (p_size == 3) {
                        mt->type = MD_QUOTE_L;
                    } else {
                        memmove(mt->str + 3, mt->str, strlen(mt->str));
                        int i = 3;
                        mt->m_size += i;
                        for (i = i - 1; i >= 0; --i) {
                            mt->str[i] = '`';
                        }
                        mt->type = MD_PLAIN;
                        fseek(f, -p_size, SEEK_CUR);
                    }
                } else if (p_size == 1) {
                    read_token(mt, f);

                    if (fread(&fc, 1, 1, f)) {
                        if (fc == '`') {
                            if (!strcmp(mt->str, "TOC")) {
                                mt->type = MD_PLAIN;
                                mt->m_size = 0;
                                memset(mt->str, '\0', strlen(mt->str));
                                TOC = true;
                                return;
                            } else {
                                mt->type = MD_QUOTE_S;
                            }
                        } else {
                            mt->type = MD_PLAIN;//已经到了结束
                            memmove(mt->str + 1, mt->str, strlen(mt->str));
                            mt->m_size = 1;
                            mt->str[0] = '`';
                            fseek(f, -1, SEEK_CUR);
                        }
                    } else {
                        memmove(mt->str + p_size + 1, mt->str, strlen(mt->str));
                        mt->m_size += 1;
                        mt->str[0] = '`';
                        mt->type = MD_PLAIN;
                        fseek(f, -p_size, SEEK_CUR);
                    }
                } else {
                    read_token(mt, f);
                    memmove(mt->str + p_size, mt->str, strlen(mt->str));
                    int i = p_size;
                    mt->m_size += i;
                    for (i = i - 1; i >= 0; --i) {
                        mt->str[i] = '`';

                    }
                    mt->type = MD_PLAIN;
                }
                break;
            }
            case '>': {
                mt->type = MD_BLOCK;
                mt->m_size = 0;
                break;
            }
            case '#': {
                fseek(f, -1, SEEK_CUR);
                int sharp_num = 0;
                while (true) {
                    if (fread(&fc, 1, 1, f)) {
                        if (fc == '#') {
                            sharp_num++;
                            continue;
                        } else {
                            fseek(f, -1, SEEK_CUR);
                            break;
                        }
                    } else {
                        //仅有#号
                        mt->type = MD_PLAIN;
                        while (sharp_num--) {
                            mt->str[mt->m_size++] = '#';
                        }
                        return;
                    }
                }
                read_token(mt, f);
                if (!strcmp(mt->str, "")) {
                    mt->type = MD_PLAIN;
                    while (sharp_num--) {
                        mt->str[mt->m_size++] = '#';
                    }
                } else {
                    init_head_nodeList(&list);
                    switch (sharp_num) {
                        case 1:
                            mt->type = MD_HEAD1;
                            head_node hn;
                            hn.level = HEAD1;
                            strcpy(hn.head_content, mt->str);
                            nodelist_insert(&list, hn);
                            return;
                        case 2:
                            mt->type = MD_HEAD2;
                            head_node hn2;
                            hn2.level = HEAD2;
                            strcpy(hn2.head_content, mt->str);
                            nodelist_insert(&list, hn2);
                            return;
                        case 3:
                            mt->type = MD_HEAD3;
                            head_node hn3;
                            hn3.level = HEAD3;
                            strcpy(hn3.head_content, mt->str);
                            nodelist_insert(&list, hn3);
                            return;
                        case 4:
                            mt->type = MD_HEAD4;
                            head_node hn4;
                            hn4.level = HEAD4;
                            strcpy(hn4.head_content, mt->str);
                            nodelist_insert(&list, hn4);
                            return;
                        case 5:
                            mt->type = MD_HEAD5;
                            head_node hn5;
                            hn5.level = HEAD5;
                            strcpy(hn5.head_content, mt->str);
                            nodelist_insert(&list, hn5);
                            return;
                        case 6:
                            mt->type = MD_HEAD6;
                            head_node hn6;
                            hn6.level = HEAD6;
                            strcpy(hn6.head_content, mt->str);
                            nodelist_insert(&list, hn6);
                            return;
                        default:
                            //fixme
                            mt->type = MD_HEAD7;
                            head_node hn7;
                            hn7.level = HEAD7;
                            strcpy(hn7.head_content, mt->str);
                            nodelist_insert(&list, hn7);
                            return;
                    }
                }
            }
            default: {
                if (isnumber(fc)) {
                    char c = fc;
                    if (fread(&fc, 1, 1, f)) {
                        if (fc == '.') {
                            mt->type = MD_PLAIN;
                            char n_str[6] = SPACE;
                            for (int i = 0; i < 6; ++i) {
                                for (int j = 0; j < 6; ++j) {
                                    mt->str[mt->m_size++] = n_str[j];
                                }
                            }
                            mt->str[mt->m_size++] = c;
                            mt->str[mt->m_size++] = fc;
                            for (int i = 0; i < 2; ++i) {
                                for (int j = 0; j < 6; ++j) {
                                    mt->str[mt->m_size++] = n_str[j];
                                }
                            }
                            read_token(mt, f);
                            break;
                        } else {
                            fseek(f, -1, SEEK_CUR);
                            mt->str[mt->m_size++] = c;
                            mt->type = MD_PLAIN;
                        }
                    } else {
                        mt->type = MD_PLAIN;
                        mt->str[mt->m_size++] = fc;
                    }
                } else {
                    /**普通字符*/
                    fseek(f, -1, SEEK_CUR);
                    read_token(mt, f);
                    mt->type = MD_PLAIN;
                }
            }
        }
    } else {
        return;
    }
}





