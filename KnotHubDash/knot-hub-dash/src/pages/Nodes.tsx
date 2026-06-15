import styles from './Nodes.module.css';
import deleteIcon from '../assets/delete.svg';
import startIcon from '../assets/start.svg';
import stopIcon from '../assets/stop.svg';
import settingIcon from '../assets/setting.svg';
import homeIcon from '../assets/home.svg';

export default function Nodes() {
    const nodesData = [
        {
            id: 'node-01',
            appId: 'com.example.node01',
            role: '主控',
            status: '运行中',
            hotRole: '主节点',
            author: '课堂助手团队',
            version: 'v1.2.0',
            statusClass: 'status-badge',
        },
        {
            id: 'node-02',
            appId: 'com.example.node02',
            role: '工作节点',
            status: '运行中',
            hotRole: '热备从机',
            author: '课堂助手团队',
            version: 'v1.1.0',
            statusClass: 'status-badge',
        },
        {
            id: 'node-03',
            appId: 'com.example.node03',
            role: '备份节点',
            status: '停止',
            hotRole: '待命',
            author: '课堂助手团队',
            version: 'v0.9.0',
            statusClass: 'status-badge warning',
        },
    ];

    const handleItemClick = (nodeId: string) => {
        window.dispatchEvent(new CustomEvent('update-preview', { detail: { type: 'node', id: nodeId } }));
    };

    const handleAction = (action: string, nodeId: string, e: React.MouseEvent) => {
        e.stopPropagation();
        alert(`[演示] 对节点 ${nodeId} 执行操作: ${action}`);
        // 后续可调用 Tauri 命令
    };

    return (
        <>
            <div style={{ marginBottom: 16 }}>
                <h1 style={{ fontSize: 22, fontWeight: 500 }}>节点列表</h1>
                <p style={{ color: '#6c757d' }}>服务节点管理 · 插入式结构</p>
            </div>
            <div className="section">
                <div className="section-header">
                    <div className="section-title">节点管理</div>
                    <button className="btn btn-sm" onClick={() => alert('安装插件（演示）')}>
                        安装插件
                    </button>
                </div>
                <div className={styles.nodesList}>
                    {nodesData.map((node) => (
                        <div
                            key={node.id}
                            className={styles.nodeCard}
                            onClick={() => handleItemClick(node.id)}
                        >
                            {/* 上区域：APP ID（左） | 状态 + 删除按钮（右） */}
                            <div className={styles.top}>
                                <span className={styles.appId}>{node.appId}</span>
                                <div className={styles.topRight}>
                                    <span className={node.statusClass}>{node.status}</span>
                                    <button
                                        className={`btn btn-sm ${styles.iconBtn} ${styles.deleteIcon} btn-danger ${styles.noBorder}`}
                                        aria-label="删除"
                                        onClick={(e) => handleAction('delete', node.id, e)}
                                    >
                                        <img src={deleteIcon} alt="删除" className={styles.deleteSvg} />
                                    </button>
                                </div>
                            </div>

                            {/* 中区域：图标 + 节点名称（左） | 操作按钮组（右） */}
                            <div className={styles.middle}>
                                <div className={styles.infoLeft}>
                                    <span className={styles.icon}>🖥️</span>
                                    <span className={styles.name}>{node.id}</span>
                                </div>
                                <div className={styles.actions}>
                                    {/* 启动/停止按钮：根据状态切换图标 */}
                                    <button
                                        className={`btn btn-sm ${styles.iconBtn} ${styles.noBorder}`}
                                        aria-label={node.status === '运行中' ? '停止' : '启动'}
                                        onClick={(e) => handleAction(node.status === '运行中' ? 'stop' : 'start', node.id, e)}
                                    >
                                        <img
                                            src={node.status === '运行中' ? stopIcon : startIcon}
                                            alt={node.status === '运行中' ? '停止' : '启动'}
                                            className={styles.actionIcon}
                                        />
                                    </button>
                                    {/* 设置按钮 */}
                                    <button
                                        className={`btn btn-sm ${styles.iconBtn} ${styles.noBorder}`}
                                        aria-label="设置"
                                        onClick={(e) => handleAction('settings', node.id, e)}
                                    >
                                        <img src={settingIcon} alt="设置" className={styles.actionIcon} />
                                    </button>
                                    {/* 主页按钮 */}
                                    <button
                                        className={`btn btn-sm ${styles.iconBtn} ${styles.noBorder}`}
                                        aria-label="主页"
                                        onClick={(e) => handleAction('home', node.id, e)}
                                    >
                                        <img src={homeIcon} alt="主页" className={styles.actionIcon} />
                                    </button>
                                </div>
                            </div>

                            {/* 下区域：作者信息（左） | 版本号（右） */}
                            <div className={styles.bottom}>
                                <span className={styles.author}>{node.author}</span>
                                <span className={styles.version}>{node.version}</span>
                            </div>
                        </div>
                    ))}
                </div>
            </div>
        </>
    );
}