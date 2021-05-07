#include "battery.h"


class Battery{
    private:
        int mv;

        void activateReading();
        void readmv();

    public:
        Battery();
        byte soc();
        int mv();
};
