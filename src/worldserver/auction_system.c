/* auction_system.c -- Auction house for WoW 3.3.5a emulator */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_AUCTIONS 5000
#define AUCTION_FEE_PCT 5        /* 5% deposit */
#define AUCTION_CUT_PCT 5        /* 5% AH cut */

typedef enum { AH_ALLIANCE=2, AH_HORDE=6, AH_NEUTRAL=7 } AuctionHouseId;
typedef enum { AUCTION_SHORT=1, AUCTION_MEDIUM=2, AUCTION_LONG=3 } AuctionDuration;

typedef struct Auction {
    uint32_t id;
    uint64_t ownerGuid;
    uint64_t bidderGuid;
    uint32_t itemEntry;
    uint32_t stackCount;
    uint64_t itemGuid;
    uint64_t startBid;
    uint64_t buyout;
    uint64_t currentBid;
    time_t   startTime;
    time_t   expireTime;
    AuctionHouseId house;
    bool     active;
    bool     sold;
} Auction;

static Auction g_auctions[MAX_AUCTIONS];
static uint32_t g_nextAuctionId = 1;

void AuctionSystem_Init(void) {
    memset(g_auctions, 0, sizeof(g_auctions));
    g_nextAuctionId = 1;
    printf("[Auction] Auction house initialized (max %d listings)\n", MAX_AUCTIONS);
}

static Auction* _findFreeSlot(void) {
    for (int i = 0; i < MAX_AUCTIONS; i++)
        if (!g_auctions[i].active) return &g_auctions[i];
    return NULL;
}

uint32_t AuctionSystem_Create(uint64_t ownerGuid, uint32_t itemEntry, uint32_t stackCount,
                               uint64_t startBid, uint64_t buyout, AuctionDuration duration,
                               AuctionHouseId house) {
    Auction* a = _findFreeSlot();
    if (!a) return 0;
    memset(a, 0, sizeof(Auction));
    a->id = g_nextAuctionId++;
    a->ownerGuid = ownerGuid;
    a->itemEntry = itemEntry;
    a->stackCount = stackCount;
    a->startBid = startBid;
    a->buyout = buyout;
    a->currentBid = 0;
    a->startTime = time(NULL);
    a->house = house;
    a->active = true;

    /* Duration: short=12h, medium=24h, long=48h */
    int hours = (duration == AUCTION_SHORT) ? 12 : (duration == AUCTION_MEDIUM) ? 24 : 48;
    a->expireTime = a->startTime + hours * 3600;

    printf("[Auction] Listed item %u x%u by %llu (bid: %llu, buyout: %llu)\n",
           itemEntry, stackCount, (unsigned long long)ownerGuid,
           (unsigned long long)startBid, (unsigned long long)buyout);
    return a->id;
}

bool AuctionSystem_Bid(uint32_t auctionId, uint64_t bidderGuid, uint64_t bidAmount) {
    for (int i = 0; i < MAX_AUCTIONS; i++) {
        if (g_auctions[i].id == auctionId && g_auctions[i].active) {
            Auction* a = &g_auctions[i];
            if (bidAmount <= a->currentBid) return false;
            if (a->buyout > 0 && bidAmount >= a->buyout) {
                a->bidderGuid = bidderGuid;
                a->currentBid = a->buyout;
                a->sold = true;
                a->active = false;
                printf("[Auction] Buyout! Item %u sold to %llu for %llu\n",
                       a->itemEntry, (unsigned long long)bidderGuid, (unsigned long long)a->buyout);
                return true;
            }
            a->bidderGuid = bidderGuid;
            a->currentBid = bidAmount;
            printf("[Auction] Bid %llu on item %u by %llu\n",
                   (unsigned long long)bidAmount, a->itemEntry, (unsigned long long)bidderGuid);
            return true;
        }
    }
    return false;
}

bool AuctionSystem_Cancel(uint32_t auctionId, uint64_t ownerGuid) {
    for (int i = 0; i < MAX_AUCTIONS; i++) {
        if (g_auctions[i].id == auctionId && g_auctions[i].active && g_auctions[i].ownerGuid == ownerGuid) {
            g_auctions[i].active = false;
            return true;
        }
    }
    return false;
}

/* Process expired auctions */
void AuctionSystem_Update(void) {
    time_t now = time(NULL);
    for (int i = 0; i < MAX_AUCTIONS; i++) {
        Auction* a = &g_auctions[i];
        if (!a->active) continue;
        if (a->expireTime <= now) {
            if (a->currentBid > 0 && a->bidderGuid != 0) {
                a->sold = true;
                printf("[Auction] Auction %u expired -- sold to %llu for %llu\n",
                       a->id, (unsigned long long)a->bidderGuid, (unsigned long long)a->currentBid);
            } else {
                printf("[Auction] Auction %u expired -- no bids, returning item to %llu\n",
                       a->id, (unsigned long long)a->ownerGuid);
            }
            a->active = false;
        }
    }
}

/* Search auctions */
int AuctionSystem_Search(AuctionHouseId house, const char* searchText,
                          uint32_t* outIds, int maxResults) {
    int count = 0;
    for (int i = 0; i < MAX_AUCTIONS && count < maxResults; i++) {
        if (!g_auctions[i].active || g_auctions[i].house != house) continue;
        if (searchText && searchText[0]) {
            /* Name search would require item template lookup -- simplified */
        }
        outIds[count++] = g_auctions[i].id;
    }
    return count;
}

Auction* AuctionSystem_GetById(uint32_t auctionId) {
    for (int i = 0; i < MAX_AUCTIONS; i++)
        if (g_auctions[i].id == auctionId) return &g_auctions[i];
    return NULL;
}
