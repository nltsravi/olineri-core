import { registerPlugin } from '@capacitor/core';
import type { OlineriCapacitorPlugin } from './definitions';

const Olineri = registerPlugin<OlineriCapacitorPlugin>('Olineri', {
  web: () => import('./web').then(m => new m.OlineriWeb()),
});

export * from './definitions';
export { Olineri };
