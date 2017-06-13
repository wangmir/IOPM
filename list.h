#ifndef _LINUX_LIST_H 
#define _LINUX_LIST_H 

struct list_head {
	struct list_head *next, *prev;
};

#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); \
	(ptr)->prev = (ptr); \
} while (0)

void __list_add(struct list_head *_new, struct list_head *prev, struct list_head *next)
{
	next->prev = _new;
	_new->next = next;
	_new->prev = prev;
	prev->next = _new;
}

void list_add(struct list_head *_new, struct list_head *head)
{
	__list_add(_new, head, head->next);
}

void list_add_tail(struct list_head *_new, struct list_head *head)
{
	__list_add(_new, head->prev, head);
}

void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = (void *)0;
	entry->prev = (void *)0;
}

void list_del_init(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

void list_move(struct list_head *list, struct list_head *head)
{
	__list_del(list->prev, list->next);
	list_add(list, head);
}

void list_move_tail(struct list_head *list,
struct list_head *head)
{
	__list_del(list->prev, list->next);
	list_add_tail(list, head);
}

int list_empty(struct list_head *head)
{
	return head->next == head;
}

void __list_splice(struct list_head *list,
struct list_head *head)
{
	struct list_head *first = list->next;
	struct list_head *last = list->prev;
	struct list_head *at = head->next;

	first->prev = head;
	head->next = first;

	last->next = at;
	at->prev = last;
}

void list_splice(struct list_head *list, struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head);
}

void list_splice_init(struct list_head *list,
struct list_head *head)
{
	if (!list_empty(list)) {
		__list_splice(list, head);
		INIT_LIST_HEAD(list);
	}
}

#define list_entry(ptr, type, member) \
((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define list_first_entry(ptr, type, member) \
list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) \
list_entry((ptr)->prev, type, member)

#define list_for_each(pos, head) \
for (pos = (head)->next; pos != (head); \
	pos = pos->next)

#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); \
		pos = pos->prev)

#define list_for_each_safe(pos, n, head) \
		for (pos = (head)->next, n = pos->next; pos != (head); \
			pos = n, n = pos->next)

// because there are no 'typeof' at the visual c, we add type
#define list_for_each_entry(type, pos, head, member)                          \
			for (pos = list_entry((head)->next, type, member)      \
				;                        \
				&pos->member != (head);                                    \
				pos = list_entry(pos->member.next, type, member)  \
				)

#endif 
