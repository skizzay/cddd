#ifndef CDDD_EVENT_H__
#define CDDD_EVENT_H__

namespace cddd {

class event {
public:
	virtual ~event() = default;

	virtual void cancel() = 0;
};

}


#endif
