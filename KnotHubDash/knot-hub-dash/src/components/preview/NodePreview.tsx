import React, { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';

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

  const handleAutoStartChange = async (checked: boolean) => {
    if (!detail) return;
    try {
      setUpdating(true);
      // TODO: 调用后端命令保存
      await invoke('set_node_autostart', { nodeId, autoStart: checked });
      setDetail({ ...detail, autoStart: checked });
    } catch (err: any) {
      alert(`设置失败: ${err.message}`);
    } finally {
      setUpdating(false);
    }
  };

  useEffect(() => {
    fetchDetail();
  }, [nodeId]);

  if (loading) return <div className="preview-loading">加载节点信息中...</div>;
  if (error) return <div className="preview-error">错误: {error}</div>;
  if (!detail) return null;

  return (
    <div className="node-preview">
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
    </div>
  );
};

export default NodePreview;