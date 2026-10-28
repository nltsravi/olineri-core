import { registerPlugin } from '@capacitor/core';
import type { VaaniCapacitorPlugin } from './definitions';

const Vaani = registerPlugin<VaaniCapacitorPlugin>('Vaani', {
  web: () => import('./web').then(m => new m.VaaniWeb()),
});

export * from './definitions';
export { Vaani };
