// src/components/FunctionListParser/types.ts

export type ArgType = 'static' | 'optional' | 'input';

export interface ArgBase {
  type: ArgType;
  description?: string;
}

export interface StaticArg extends ArgBase {
  type: 'static';
  value: string;
}

export interface OptionalArg extends ArgBase {
  type: 'optional';
  options: [string, string][]; // [显示名, 实际值]
}

export interface InputArg extends ArgBase {
  type: 'input';
  defaultVal?: string;
}

export type Arg = StaticArg | OptionalArg | InputArg;

export interface OpenSocketFunc {
  appID: string;
  openSocketID: string;
  description: string;
  args: Record<string, Arg>;
  returns: [string, string][];
}

export interface SignalFunc {
  appID: string;
  signalID: string;
  description: string;
  returns: Record<string, { description: string; verification?: string }>;
}

export interface FunctionManifest {
  appName: string;
  openSocket: Record<string, OpenSocketFunc>;
  signal: Record<string, SignalFunc>;
}