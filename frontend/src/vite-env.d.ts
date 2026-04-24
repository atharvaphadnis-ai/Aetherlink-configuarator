/// <reference types="vite/client" />

interface Navigator {
  serial: {
    requestPort(): Promise<SerialPort>;
  };
}
