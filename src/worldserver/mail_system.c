/* mail_system.c -- In-game mail system for WoW 3.3.5a emulator */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_MAIL_PER_PLAYER 100
#define MAX_MAIL_ITEMS 12
#define MAIL_EXPIRE_DAYS 30

typedef enum { MAIL_NORMAL=0, MAIL_AUCTION=2, MAIL_CREATURE=3, MAIL_GAMEOBJECT=4, MAIL_ITEM=5 } MailType;
typedef enum { MAIL_CHECK_NONE=0, MAIL_CHECK_READ=1, MAIL_CHECK_RETURNED=2, MAIL_CHECK_COPIED=4 } MailCheckMask;

typedef struct MailItem {
    uint64_t itemGuid;
    uint32_t itemEntry;
    uint32_t stackCount;
} MailItem;

typedef struct Mail {
    uint32_t    id;
    MailType    type;
    uint64_t    senderGuid;
    uint64_t    receiverGuid;
    char        subject[128];
    char        body[512];
    MailItem    items[MAX_MAIL_ITEMS];
    int         itemCount;
    uint64_t    money;
    uint64_t    cod;              /* cash on delivery */
    time_t      sentTime;
    time_t      expireTime;
    uint32_t    checkMask;
    bool        read;
    bool        deleted;
} Mail;

typedef struct Mailbox {
    Mail     mails[MAX_MAIL_PER_PLAYER];
    int      count;
    uint32_t nextMailId;
} Mailbox;

static uint32_t g_nextMailId = 1;

void MailSystem_Init(void) {
    g_nextMailId = 1;
    printf("[Mail] Mail system initialized\n");
}

void Mailbox_Init(Mailbox* mb) {
    memset(mb, 0, sizeof(Mailbox));
}

bool MailSystem_Send(Mailbox* receiverBox, uint64_t senderGuid, uint64_t receiverGuid,
                     const char* subject, const char* body, uint64_t money, uint64_t cod) {
    if (!receiverBox || receiverBox->count >= MAX_MAIL_PER_PLAYER) return false;
    Mail* m = &receiverBox->mails[receiverBox->count++];
    m->id = g_nextMailId++;
    m->type = MAIL_NORMAL;
    m->senderGuid = senderGuid;
    m->receiverGuid = receiverGuid;
    strncpy(m->subject, subject ? subject : "", 127);
    strncpy(m->body, body ? body : "", 511);
    m->money = money;
    m->cod = cod;
    m->sentTime = time(NULL);
    m->expireTime = m->sentTime + MAIL_EXPIRE_DAYS * 86400;
    m->read = false;
    m->deleted = false;
    printf("[Mail] Sent: '%s' from %llu to %llu (%llu copper)\n",
           m->subject, (unsigned long long)senderGuid,
           (unsigned long long)receiverGuid, (unsigned long long)money);
    return true;
}

bool MailSystem_AttachItem(Mailbox* mb, uint32_t mailId, uint64_t itemGuid,
                           uint32_t itemEntry, uint32_t stackCount) {
    for (int i = 0; i < mb->count; i++) {
        if (mb->mails[i].id == mailId && mb->mails[i].itemCount < MAX_MAIL_ITEMS) {
            MailItem* mi = &mb->mails[i].items[mb->mails[i].itemCount++];
            mi->itemGuid = itemGuid;
            mi->itemEntry = itemEntry;
            mi->stackCount = stackCount;
            return true;
        }
    }
    return false;
}

int MailSystem_GetUnread(Mailbox* mb) {
    int count = 0;
    for (int i = 0; i < mb->count; i++)
        if (!mb->mails[i].read && !mb->mails[i].deleted) count++;
    return count;
}

Mail* MailSystem_GetMail(Mailbox* mb, uint32_t mailId) {
    for (int i = 0; i < mb->count; i++)
        if (mb->mails[i].id == mailId && !mb->mails[i].deleted) return &mb->mails[i];
    return NULL;
}

void MailSystem_MarkRead(Mailbox* mb, uint32_t mailId) {
    Mail* m = MailSystem_GetMail(mb, mailId);
    if (m) { m->read = true; m->checkMask |= MAIL_CHECK_READ; }
}

void MailSystem_Delete(Mailbox* mb, uint32_t mailId) {
    Mail* m = MailSystem_GetMail(mb, mailId);
    if (m) m->deleted = true;
}

/* Return expired/deleted mail to sender */
void MailSystem_CleanExpired(Mailbox* mb) {
    time_t now = time(NULL);
    for (int i = mb->count - 1; i >= 0; i--) {
        if (mb->mails[i].deleted || mb->mails[i].expireTime < now) {
            mb->mails[i] = mb->mails[mb->count - 1];
            mb->count--;
        }
    }
}
