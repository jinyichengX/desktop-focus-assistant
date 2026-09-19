/*
 * MIT License
 *
 * Copyright (c) 2024~2025 JinYiCheng
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _SMLIST_H
#define _SMLIST_H

#ifndef _offsetof
#define _offsetof(TYPE, MEMBER)   ((size_t) &((TYPE *)0)->MEMBER)
#endif
#ifndef _container_of
#define _container_of(ptr, type, member) (type *)((char *)ptr -offsetof(type,member))
#endif
#ifndef _list_entry
#define _list_entry(ptr, type, member) _container_of(ptr, type, member)
#endif

typedef struct listnode
{
    struct listnode * next;
}node_t; 

/* init list */
static inline void _list_init(node_t * list)
{
    list->next = NULL;
}

/* init node */
static inline void _list_node_init(node_t * node)
{
    node->next = NULL;
}

/* add node to list front */
static inline void _list_add(node_t * list, node_t * node)
{
    node->next = list->next;
    list->next = node;
}

/* add node after one node */
static inline void _list_add_after(node_t * node,\
                      node_t * prev)
{
    node->next = prev->next;
    prev->next = node;
}

/* remove first node */
static inline void _list_del(node_t * list)
{
    list->next = list->next->next;
}

/* look look if list is empty */
static inline int _list_empty(node_t * list)
{
    return (list->next == NULL);
}

/* traverse list */
#define _list_traverse(pidx, plist)\
        for((pidx) = (plist)->next; (pidx); (pidx) = (pidx)->next)

#endif