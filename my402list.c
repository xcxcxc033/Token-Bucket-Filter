#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "cs402.h"

#include "my402list.h"

int My402ListLength(My402List* templist)
{

    return templist-> num_members;
}

int My402ListEmpty(My402List* templist)
{
    if(templist->anchor.next == &templist->anchor &&
            templist->anchor.prev == &templist->anchor)
        return 1;
    else
        return 0;
}

int My402ListAppend (My402List* templist, void *obj)
{
    My402ListElem *newElem = (My402ListElem*)malloc(sizeof(My402ListElem));
    if(newElem!=NULL)
        newElem->obj = obj;
    else
        return FALSE;
    newElem->next = &templist->anchor;
    newElem->prev = templist->anchor.prev;

    templist->anchor.prev->next = newElem;
    templist->anchor.prev = newElem;

    templist-> num_members++;
    return 1;
};

int My402ListPrepend(My402List* templist, void *obj)
{
    My402ListElem *newElem = (My402ListElem*)malloc(sizeof(My402ListElem));
    if(newElem!=NULL)
        newElem->obj = obj;
    else
        return FALSE;
    newElem->obj = obj;
    newElem->next = templist->anchor.next;
    newElem->prev = &templist->anchor;

    templist->anchor.next->prev = newElem;
    templist->anchor.next = newElem;
    templist->num_members++;
    return 1;
};

void My402ListUnlink (My402List* templist,My402ListElem *elem)
{
    elem->prev->next = elem ->next;
    elem->next->prev = elem ->prev;
    templist->num_members--;
    return;
};

void My402ListUnlinkAll (My402List* templist)
{
    if(My402ListEmpty(templist))
    {
        return;
    }
    My402ListElem *temp = templist->anchor.next;

    while(temp!=&templist->anchor)
    {
        temp = temp->next;
        free(temp->prev);
    }

    templist->anchor.next = templist->anchor.prev = &templist->anchor;
    templist->num_members = 0;
};

int My402ListInsertBefore(My402List* templist, void *obj, My402ListElem *elem)
{
    My402ListElem *newElem = (My402ListElem*)malloc(sizeof(My402ListElem));
    newElem->obj = obj;
    newElem->next = elem;
    newElem->prev = elem->prev;

    elem->prev->next = newElem;
    elem->prev = newElem;
    templist->num_members++;
    return 1;
};

int My402ListInsertAfter(My402List* templist, void *obj, My402ListElem *elem)
{
    My402ListElem *newElem = (My402ListElem*)malloc(sizeof(My402ListElem));
    newElem->obj = obj;
    newElem->next = elem->next;
    newElem->prev = elem;

    elem->next->prev = newElem;
    elem->next = newElem;
    templist->num_members++;
    return 1;
};

My402ListElem *My402ListFirst(My402List* templist)
{

    if (My402ListEmpty(templist))
        return NULL;
    else
        return templist->anchor.next;
};

My402ListElem *My402ListLast(My402List* templist)
{

    if (My402ListEmpty(templist))
        return NULL;
    else
        return templist->anchor.prev;
};

My402ListElem *My402ListNext(My402List* templist, My402ListElem *cur)
{
 //   if(My402ListEmpty(templist))
  //      return NULL;

    if(cur == templist->anchor.prev)
        return NULL;
    else
        return cur->next;
};

My402ListElem *My402ListPrev(My402List* templist, My402ListElem *cur)
{
    if(cur == templist->anchor.next)
        return NULL;
    else
        return cur->prev;
};

My402ListElem *My402ListFind(My402List* templist, void *obj)
{
    My402ListElem *temp =(My402ListElem*)malloc(sizeof(My402ListElem));

    temp= templist->anchor.next;

    while(temp != &templist->anchor)
    {
        if(temp->obj == obj )
        {
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
};

int My402ListInit(My402List* templist)
{
    templist->anchor.next = &templist->anchor;
    templist->anchor.prev = &templist->anchor;
    templist->num_members = 0;
    return 1;
};


