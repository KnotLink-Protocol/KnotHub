// src/hooks/useManifest.ts
import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import type { FunctionManifest } from '../components/FunctionListParser/types';

export function useManifest(pluginName: string | null) {
  const [manifest, setManifest] = useState<FunctionManifest | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    if (!pluginName) {
      setManifest(null);
      setLoading(false);
      setError(null);
      return;
    }

    let cancelled = false;
    const load = async () => {
      setLoading(true);
      setError(null);
      try {
        const data = await invoke<FunctionManifest>('get_node_manifest', { pluginName });
        if (!cancelled) {
          setManifest(data);
          // 如果返回的清单中 openSocket 为空，则显示空状态
        }
      } catch (err: any) {
        if (!cancelled) setError(err.message || '加载功能清单失败');
      } finally {
        if (!cancelled) setLoading(false);
      }
    };
    load();
    return () => { cancelled = true; };
  }, [pluginName]);

  return { manifest, loading, error };
}