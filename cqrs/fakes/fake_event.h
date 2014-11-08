#ifndef CDDD_CQRS_FAKE_EVENT_H__
#define CDDD_CQRS_FAKE_EVENT_H__

struct fake_event {
   enum { cache_line_size = 64 };
   double d;
   int i;
   char c[cache_line_size-(alignof(d) + alignof(i))];
};

#endif
