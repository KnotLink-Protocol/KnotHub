import React, { useState } from 'react';
import type { FunctionManifest } from './types';
import styles from './FunctionListDoc.module.css';

interface FunctionListDocProps {
  manifest: FunctionManifest;
}

const FunctionListDoc: React.FC<FunctionListDocProps> = ({ manifest }) => {
  const funcNames = Object.keys(manifest.openSocket);

  // 存储每个功能的展开状态，默认为 true（展开）
  const [expanded, setExpanded] = useState<Record<string, boolean>>(() => {
    const state: Record<string, boolean> = {};
    funcNames.forEach(name => { state[name] = false; });
    return state;
  });

  const toggleExpand = (funcName: string) => {
    setExpanded(prev => ({ ...prev, [funcName]: !prev[funcName] }));
  };

  return (
    <div className={styles.docContainer}>
      <h4 className={styles.docTitle}>功能清单文档</h4>
      {funcNames.length === 0 && <div className={styles.noFunc}>该节点暂无功能</div>}
      {funcNames.map(name => {
        const func = manifest.openSocket[name];
        const isExpanded = expanded[name] ?? true; // 默认展开
        return (
          <div key={name} className={styles.funcDoc}>
            <div className={styles.funcHeader} onClick={() => toggleExpand(name)}>
              <div className={styles.funcHeaderLeft}>
                <span className={styles.expandIcon}>{isExpanded ? '▾' : '▸'}</span>
                <span className={styles.funcName}>{name}</span>
                <span className={styles.funcDesc}>{func.description}</span>
              </div>
            </div>
            {isExpanded && (
              <div className={styles.funcDetails}>
                <div className={styles.funcMeta}>
                  <div><strong>App ID</strong> {func.appID}</div>
                  <div><strong>OpenSocket ID</strong> {func.openSocketID}</div>
                </div>
                <div className={styles.argsDoc}>
                  <strong>参数</strong>
                  {Object.keys(func.args).length === 0 && <span className={styles.noArgs}>无</span>}
                  {Object.entries(func.args).map(([argName, arg]) => (
                    <div key={argName} className={styles.argItem}>
                      <span className={styles.argName}>{argName}</span>
                      <span className={styles.argType}>（{arg.type}）</span>
                      {arg.description && <span className={styles.argDesc}>：{arg.description}</span>}
                      {arg.type === 'optional' && arg.options && (
                        <div className={styles.argOptions}>
                          可选值：
                          {arg.options.map(([label, value]) => (
                            <span key={value} className={styles.optionTag}>{label}（{value}）</span>
                          ))}
                        </div>
                      )}
                      {arg.type === 'input' && arg.defaultVal && (
                        <div className={styles.argDefault}>默认值：{arg.defaultVal}</div>
                      )}
                      {arg.type === 'static' && (
                        <div className={styles.argStatic}>固定值：{arg.value}</div>
                      )}
                    </div>
                  ))}
                </div>
                {func.returns.length > 0 && (
                  <div className={styles.returnsDoc}>
                    <strong>返回值</strong>
                    {func.returns.map(([name, desc]) => (
                      <div key={name} className={styles.returnItem}>
                        <span className={styles.returnName}>{name}</span>
                        {desc && <span className={styles.returnDesc}>：{desc}</span>}
                      </div>
                    ))}
                  </div>
                )}
              </div>
            )}
          </div>
        );
      })}
    </div>
  );
};

export default FunctionListDoc;