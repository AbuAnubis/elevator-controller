#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
// #include "config.h"

#define NFLOOR 50  // number of floors
#define NELEV 16  // number of elevators

#define CDIST 1  // metric cost of one floor distance

#define floor_t int16_t  // signed - negative values mean "none"
#define elevid_t int8_t  // signed - negative values mean "none"
#define qpos_t uint16_t
#define QPOS_MAX UINT16_MAX

#define metric_t uint32_t

struct request {
    floor_t floor_id;
    int8_t dir;
    qpos_t qpos;
};

typedef struct elevator {
    elevid_t id;
    floor_t pos;  // current position
    floor_t target;  // where the elevator is going
    uint8_t* buttons;  // which buttons are pressed
    struct request served;  // last served request
    struct request newest;  // newest request
    metric_t maxmetr;  // metric of the "newest"
} elevator;

typedef struct floor {
    floor_t id;
    elevid_t upelev;  // chosen elevator for going up (none, if negative)
    qpos_t upqueue;  // position in queue of the "up" button
    metric_t upmetr;  // metric for chosen elevator
    elevid_t dnelev;  // chosen elevator for going down (none, if negative)
    qpos_t dnqueue;  // position in queue of the "down" button
    metric_t dnmetr;  // metric for chosen elevator
} floor;


void init(elevator* elevs, floor* floors) {
    for(int i=0; i<NELEV; ++i) {
        elevs[i].id = i;
        elevs[i].pos = 0;
        elevs[i].target = 0;
        elevs[i].buttons = (uint8_t*) calloc(NFLOOR / 8 + 1, 1);
        elevs[i].served.floor_id = -1;
        elevs[i].served.qpos = 0;
        elevs[i].newest.floor_id = -1;
        elevs[i].newest.qpos = 0;
        elevs[i].maxmetr = 0;
    }
    for(int i=0; i<NFLOOR; ++i) {
        floors[i].id = i;
        floors[i].upelev = -1;
        floors[i].upqueue = 0;
        floors[i].upmetr = 0;
        floors[i].dnelev = -1;
        floors[i].dnqueue = 0;
        floors[i].dnmetr = 0;
    }
}

qpos_t qrel(qpos_t qpos, elevator elev) {
    if(qpos >= elev.served.qpos) {
        return qpos - elev.served.qpos;
    } else {
        return QPOS_MAX - elev.served.qpos + qpos + 1;
    }
}

qpos_t qnext(qpos_t qcurr) {
    if(qcurr < QPOS_MAX) {
        return qcurr + 1;
    } else {
        return 0;  // the queue works in a circular manner
    }
}

int request_eq(struct request a, struct request b) {
    return a.floor_id == b.floor_id && a.dir == b.dir;
}

/**
 * Checks if current elevator ride is suitable for the Passenger:
 * 1. elevator is currently not in use
 * 2. elevator goes towards the Passenger, but will finish its course before reaching the Passenger
 * 3. elevator goes the same way the Passenger wants to go and the elevator will reach the Passenger before finishing its course
 * 
 * @returns true if any of the conditions are met, false otherwise
**/
int check_elev(floor fl, int dir, elevator elev) {
    #define _pos elev.pos
    #define _trg elev.target
    #define _fid fl.id
    // condition 1
    if(_pos == _trg) return 1;
    // condition 2
    else if((_pos <= _trg && _trg <= _fid) || (_pos >= _trg && _trg >= _fid)) return 1;
    // condition 3
    else if((dir > 0 && _pos <= _fid && _fid <= _trg) || (dir < 0 && _pos >= _fid && _fid >= _trg)) return 1;
    // no condition met
    else return 0;
    #undef _pos
    #undef _trg
    #undef _fid
}

metric_t metric_dist(floor_t a, floor_t b) {
    if(a >= b) {
        return CDIST * (a - b);
    } else {
        return CDIST * (b - a);
    }
}

metric_t metric(floor fl, int dir, elevator elev) {
    if(request_eq(elev.served, elev.newest)) {  // the elevator completed all previous requests
        if(check_elev(fl, dir, elev)) {
            return metric_dist(elev.pos, fl.id);
        } else {  // elevator goes "the wrong way" - Worst Case Sceanrio will be used (elevator will go as far as possible)
            floor_t wcs = (elev.pos < elev.target ? NFLOOR - 1 : 0);  // either top or bottom floor
            return metric_dist(elev.pos, wcs) + metric_dist(wcs, fl.id);
        }
    } else if(elev.newest.dir == dir && (fl.id - elev.newest.floor_id) * dir > 0) {
        // this request can be completed simultaneously with elevator's newest request
        
    }
}

void shutdown(elevator* elevs) {
    for(int i=0; i<NELEV; ++i) {
        free(elevs[i].buttons);
    }
}


int main() {
    elevator elevs[NELEV];
    floor floors[NFLOOR];
    init(elevs, floors);

    shutdown(elevs);
}
