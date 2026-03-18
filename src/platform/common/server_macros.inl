class Platform;
struct PlatformImplState;

#define DECLARE_SERVER(Server)					\
public:											\
	Server(const Server& other) = delete;		\
	Server& operator=(Server& other) = delete;	\
	Server(Server&& other) = delete;			\
	Server& operator=(Server&& other) = delete;	\
												\
private:										\
	explicit Server(PlatformImplState& state)	\
		:m_state(state)							\
	{}											\
	PlatformImplState& m_state;					\
	friend Platform

