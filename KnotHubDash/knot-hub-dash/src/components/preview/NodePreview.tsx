import React, { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
// 新增导入：功能清单解析器
import FunctionListParser from '../FunctionListParser';
import type { FunctionManifest, OpenSocketFunc } from '../FunctionListParser/types';
import FunctionListDoc from '../FunctionListParser/FunctionListDoc';

interface NodeDetail {
  pluginName: string;
  appId: string;
  author: string;
  version: string;
  description: string;
  status: string;
  autoStart: boolean;
}

interface NodePreviewProps {
  nodeId: string;
}

const NodePreview: React.FC<NodePreviewProps> = ({ nodeId }) => {
  const [detail, setDetail] = useState<NodeDetail | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [updating, setUpdating] = useState(false);

  // ---- 新增：功能清单相关状态 ----
  const [manifest, setManifest] = useState<FunctionManifest | null>(null);
  const [manifestLoading, setManifestLoading] = useState(false);
  const [manifestError, setManifestError] = useState<string | null>(null);

    // 新增视图模式状态
  const [viewMode, setViewMode] = useState<'interactive' | 'docs'>('interactive');

  const fetchDetail = async () => {
    try {
      setLoading(true);
      // TODO: 替换为真实的后端调用
      const data = await invoke<NodeDetail>('get_node_detail', { nodeId });
      const mockData: NodeDetail = {
        pluginName: 'KnotHub 核心插件',
        appId: '0x0000A001',
        author: '课堂助手团队',
        version: 'v1.2.0',
        description: '管理 KnotHub 服务中枢的核心节点',
        status: '运行中',
        autoStart: true,
      };
      setDetail(data);
      setError(null);
    } catch (err: any) {
      setError(err.message || '获取详情失败');
    } finally {
      setLoading(false);
    }
  };

  const fetchManifest = async () => {
  if (!nodeId) {
    setManifest(null);
    return;
  }
  setManifestLoading(true);
  setManifest(null);          // 立即清除旧清单，避免残留
  setManifestError(null);
  try {
    const data = await invoke<FunctionManifest>('get_node_manifest', { nodeId });
    setManifest(data);
  } catch (err: any) {
    console.error('get_node_manifest 错误详情:', err);
    setManifestError(err.message || '加载功能清单失败');
  } finally {
    setManifestLoading(false);
  }
};

  const handleAutoStartChange = async (checked: boolean) => {
    if (!detail) return;
    try {
      setUpdating(true);
      await invoke('set_node_autostart', { nodeId, autoStart: checked });
      setDetail({ ...detail, autoStart: checked });
    } catch (err: any) {
      alert(`设置失败: ${err.message}`);
    } finally {
      setUpdating(false);
    }
  };

  // ---- 新增：处理功能调用 ----
  const handleInvoke = async (func: OpenSocketFunc, args: Record<string, string>) => {
    const { appID, openSocketID } = func;
    try {
      const result = await invoke('call_open_socket', { appId: appID, openSocketId: openSocketID, args });
      alert(`调用成功：${JSON.stringify(result)}`);
    } catch (err: any) {
      alert(`调用失败：${err.message}`);
    }
  };

  useEffect(() => {
    fetchDetail();
    fetchManifest(); // ---- 新增：加载功能清单 ----
  }, [nodeId]);

  if (loading) return <div className="preview-loading">加载节点信息中...</div>;
  if (error) return <div className="preview-error">错误: {error}</div>;
  if (!detail) return null;

  return (
    <div className="node-preview">
      {/* === 原有节点详情（完全不变） === */}
      <div className="preview-field"><strong>插件名称</strong> {detail.pluginName}</div>
      <div className="preview-field"><strong>应用ID</strong> {detail.appId}</div>
      <div className="preview-field"><strong>作者</strong> {detail.author}</div>
      <div className="preview-field"><strong>版本</strong> {detail.version}</div>
      <div className="preview-field"><strong>描述</strong> {detail.description}</div>
      <div className="preview-field"><strong>状态</strong> {detail.status}</div>
      <div className="preview-field">
        <strong>自启动</strong>
        <div className="switch-wrapper" style={{ display: 'inline-block', marginLeft: '8px', verticalAlign: 'middle' }}>
          <label className="switch">
            <input
              type="checkbox"
              checked={detail.autoStart}
              onChange={(e) => handleAutoStartChange(e.target.checked)}
              disabled={updating}
            />
            <span className="slider round"></span>
          </label>
          {updating && <span style={{ marginLeft: '8px', fontSize: '12px', color: 'var(--text-secondary)' }}>保存中...</span>}
        </div>
      </div>

      {/* === 功能清单区域 === */}
      {manifestLoading && <div className="manifest-loading">加载功能清单中...</div>}
      {manifestError && <div className="manifest-error">功能清单加载失败: {manifestError}</div>}
      {manifest && Object.keys(manifest.openSocket).length > 0 && (
        <div className="manifest-section">
          <hr />
          <div className="manifest-toolbar">
            <h4 style={{ margin: '8px 0 4px 0', fontSize: '14px' }}>功能清单</h4>
            <div className="view-toggle">
              <button
                className={`view-btn ${viewMode === 'interactive' ? 'active' : ''}`}
                onClick={() => setViewMode('interactive')}
              >
                交互
              </button>
              <button
                className={`view-btn ${viewMode === 'docs' ? 'active' : ''}`}
                onClick={() => setViewMode('docs')}
              >
                文档
              </button>
            </div>
          </div>
          {viewMode === 'interactive' && (
            <FunctionListParser manifest={manifest} onInvoke={handleInvoke} compact={true} />
          )}
          {viewMode === 'docs' && (
            <FunctionListDoc manifest={manifest} />
          )}
        </div>
      )}
      {manifest && Object.keys(manifest.openSocket).length === 0 && (
        <div className="manifest-empty">该节点暂无可用功能</div>
      )}
    </div>
  );
};

export default NodePreview;