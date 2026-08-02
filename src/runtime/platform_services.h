#pragma once

class IDisplayServer;
class IOSServer;
class IIOServer;
class EventServer;

struct PlatformServices {
    IDisplayServer &display;
    IIOServer &io;
    IOSServer &os;
    EventServer &event;
};
