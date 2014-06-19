#ifndef CDDD_CQRS_FAKE_EVENT_H__
#define CDDD_CQRS_FAKE_EVENT_H__

struct fake_event {
   double d;
   int i;
   char c[64-(alignof(d) + alignof(i))];
};

#endif
