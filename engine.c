#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
// #include "config.h"

#define NFLOOR 50  // number of floors
#define NELEV 16  // number of elevators

#define CDIST 1  // metric cost of one floor distance

#define TFLOOR 1  // how many ticks it takes to move one floor
#define TSTOP 1  // how many ticks the elevator stays on a particular floor

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
    qpos_t qnext;  // queue position of the request currently being served
    metric_t maxmetr;  // metric of the "newest"
    int wait;  // how many ticks more the elevator has to wait on the stop
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
    for(elevid_t i=0; i<NELEV; ++i) {
        elevs[i].id = i;
        elevs[i].pos = 0;
        elevs[i].target = 0;
        elevs[i].buttons = (uint8_t*) calloc(NFLOOR / 8 + 1, 1);
        elevs[i].served.floor_id = -1;
        elevs[i].served.qpos = 0;
        elevs[i].newest.floor_id = -1;
        elevs[i].newest.qpos = 0;
        elevs[i].qnext = 0;
        elevs[i].maxmetr = 0;
        elevs[i].wait = 0;
    }
    for(floor_t i=0; i<NFLOOR; ++i) {
        floors[i].id = i;
        floors[i].upelev = -1;
        // floors[i].upqueue = 0;
        // floors[i].upmetr = 0;
        floors[i].dnelev = -1;
        // floors[i].dnqueue = 0;
        // floors[i].dnmetr = 0;
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
    if(elev.newest.floor_id < 0 || request_eq(elev.served, elev.newest)) {  // the elevator completed all previous requests
        if(check_elev(fl, dir, elev)) {
            return metric_dist(elev.pos, fl.id);
        } else {  // elevator goes "the wrong way" - Worst Case Sceanrio will be used (elevator will go as far as possible)
            floor_t wcs = (elev.pos < elev.target ? NFLOOR - 1 : 0);  // either top or bottom floor
            return metric_dist(elev.pos, wcs) + metric_dist(wcs, fl.id);
        }
    } else if(elev.newest.dir == dir && (fl.id - elev.newest.floor_id) * dir > 0) {
        // this request can be completed simultaneously with elevator's newest request
        return elev.maxmetr + metric_dist(elev.newest.floor_id, fl.id);
    } else {
        // this request is not compatible with elevator's newest request
        // Worst Case Sceanrio will be used (elevator will go as far as possible)
        floor_t wcs = (elev.newest.dir > 0 ? NFLOOR - 1 : 0);  // either top or bottom floor
        return elev.maxmetr + metric_dist(elev.newest.floor_id, wcs) + metric_dist(wcs, fl.id);
    }
}

elevid_t make_request(floor* fl, int dir, elevator* elevs) {
    elevid_t best_elev = 0;
    metric_t best_metric = metric(*fl, dir, elevs[0]);
    metric_t current;
    for(elevid_t i=1; i<NELEV; ++i) {
        current = metric(*fl, dir, elevs[i]);
        if(current < best_metric) {
            best_metric = current;
            best_elev = i;
        }
    }
    // best elevator found
    elevs[best_elev].newest.floor_id = fl->id;
    elevs[best_elev].newest.dir = dir;
    qpos_t new_qpos = qnext(elevs[best_elev].newest.qpos);
    elevs[best_elev].newest.qpos = new_qpos;
    if(dir > 0) {
        fl->upelev = best_elev;
        fl->upmetr = best_metric;
        fl->upqueue = new_qpos;
    } else if(dir < 0) {
        fl->dnelev = best_elev;
        fl->dnmetr = best_metric;
        fl->dnqueue = new_qpos;
    }
    return best_elev;
}

/**
 * Chooses new elevator's target
**/
void choose_target(elevator* elev, floor* fls) {
    qpos_t qnext = 0;
    floor_t fid = -1;
    for(floor_t i=0; i<NFLOOR; ++i) {
        if(fls[i].upelev == elev->id && (fid < 0 || qrel(fls[i].upqueue, *elev) < qnext)) {
            qnext = qrel(fls[i].upqueue, *elev);
            fid = i;
        }
        // if(elev->id == 0) printf("%d\n", fls[i].dnelev);
        if(fls[i].dnelev == elev->id && (fid < 0 || qrel(fls[i].dnqueue, *elev) < qnext)) {
            qnext = qrel(fls[i].dnqueue, *elev);
            fid = i;
        }
    }
    if(fid >= 0 && elev->pos == elev->target) {  // at least one floor has called this elevator and it's not in the middle of a course
        elev->target = fid;
    } else if(fid < 0) {  // no target is left
        elev->newest.floor_id = -1;
    }
    elev->qnext = qnext;
}

/**
 * Lowers metric of the elevator and all its request by a given value (to avoid counting to infinity)
**/
void lower_metric(metric_t sub, elevator* elev, floor* fls) {
    elev->maxmetr -= sub;
    for(floor_t i=0; i<NFLOOR; ++i) {
        if(fls[i].dnelev == elev->id) {
            fls[i].dnmetr -= sub;
        }
        if(fls[i].upelev == elev->id) {
            fls[i].upmetr -= sub;
        }
    }
}

void set_button(int8_t state, elevator* elev, floor_t fid) {
    floor_t block = fid >> 3;
    uint8_t mask = 0b1 << (7 - fid % 8);
    if(state) {
        elev->buttons[block] |= mask;
        if(elev->pos <= elev->target && elev->target < fid) elev->target = fid;
        else if(elev->pos >= elev->target && elev->target > fid) elev->target = fid;
    } else {
        mask = ~mask;
        elev->buttons[block] &= mask;
    }
}

int button_state(elevator elev, floor_t fid) {
    floor_t block = fid >> 3;
    uint8_t mask = 0b1 << (7 - fid % 8);
    return (elev.buttons[block] & mask) > 0;
}

void step(floor* fls, elevator* elevs) {
    #define _pos elevs[i].pos
    #define _trg elevs[i].target
    for(elevid_t i=0; i<NELEV; ++i) {
        elevs[i].wait -= 1;
        if(elevs[i].wait <= 0) {  // wait time exceeded
            if(_pos != _trg) {  // has to move towards its target
                _pos += (_pos < _trg ? 1 : -1);
                if(_pos == _trg) {  // elevator arrived at its final stop
                    set_button(0, &(elevs[i]), _pos);
                    elevs[i].wait = TSTOP;
                    if(fls[_pos].upelev == elevs[i].id && fls[_pos].upqueue == elevs[i].qnext) {
                        // elevator fulfilled its designated request
                        elevs[i].served.dir = 1;
                        elevs[i].served.floor_id = _pos;
                        elevs[i].served.qpos = fls[_pos].upqueue;
                        fls[_pos].upelev = -1;
                        lower_metric(fls[_pos].upmetr, &(elevs[i]), fls);
                    } else if(fls[_pos].dnelev == elevs[i].id && fls[_pos].dnqueue == elevs[i].qnext) {
                        // elevator fulfilled its designated request
                        elevs[i].served.dir = -1;
                        elevs[i].served.floor_id = _pos;
                        elevs[i].served.qpos = fls[_pos].dnqueue;
                        fls[_pos].dnelev = -1;
                        lower_metric(fls[_pos].dnmetr, &(elevs[i]), fls);
                    }
                    choose_target(&(elevs[i]), fls);
                } else if(_pos < _trg && fls[_pos].upelev == elevs[i].id) {  // the elevator can take someone up
                    fls[_pos].upelev = -1;
                } else if(_pos > _trg && fls[_pos].dnelev == elevs[i].id) {  // the elevator can take someone down
                    fls[_pos].dnelev = -1;
                } else {  // elevator keeps going
                    elevs[i].wait = TFLOOR;
                }
            } else {  // elevator waits for something to do
                choose_target(&(elevs[i]), fls);
            }
        }
    }
}

void print_elevators(elevator* elevs) {
    for(elevid_t i=0; i<NELEV; ++i) {
        printf("Elev. #%hhd: On floor %hd. ", i, elevs[i].pos);
        if(elevs[i].pos != elevs[i].target) printf("Going to floor %hd. ", elevs[i].target);
        int8_t stop_printed = 0;
        for(floor_t f=0; f<NFLOOR; ++f) {
            if(button_state(elevs[i], f)) {
                if(!stop_printed) {
                    printf("Stops: %hd", f);
                    stop_printed = 1;
                } else {
                    printf(", %hd", f);
                }
            }
        }
        printf("\n");
    }
}

void shutdown(elevator* elevs) {
    for(elevid_t i=0; i<NELEV; ++i) {
        free(elevs[i].buttons);
    }
}

void command(char c, elevator* elevs, floor* fls) {
    if(c == 's') {  // step
        step(fls, elevs);
    } else if(c == 'p') {
        print_elevators(elevs);
    } else if(c == 'b') {
        floor_t fid;
        elevid_t eid;
        scanf("%hhd %hd", &eid, &fid);
        set_button(1, &(elevs[eid]), fid);
    } else if(c == 'c') {
        floor_t fid;
        int8_t dir;
        scanf("%hd %hhd", &fid, &dir);
        if(dir == 0) return;
        else if(dir < 0) printf("Button 'Down' pressed on floor %hd. ", fid);
        else if(dir > 0) printf("Button 'Up' pressed on floor %hd. ", fid);
        printf("Elevator #%hhd assigned.\n", make_request(&(fls[fid]), dir, elevs));
    }
}

int main() {
    elevator elevs[NELEV];
    floor floors[NFLOOR];
    init(elevs, floors);
    //
    int8_t exit = 0;
    char c;
    while(!exit) {
        scanf("%c", &c);
        if(c == 'e') exit = 1;
        else command(c, elevs, floors);
    }
    //
    shutdown(elevs);
}
