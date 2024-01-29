#include "interrupt.hh"

Interrupt::Interrupt(Memory *memory){
    this->memory = memory;

    memory->writeByte(INTERRUPT_ENABLE, 0x00);
}

void Interrupt::toggleIME(bool enable){
    IME = enable;
}

void Interrupt::requestInterrupt(uint8_t interruptCode){
    printf("interrupt requested.\n");

    uint8_t interruptFlag = memory->readByte(INTERRUPT_FLAG);
    interruptFlag |= (1 << interruptCode);
    memory->writeByte(INTERRUPT_FLAG, interruptFlag);
}