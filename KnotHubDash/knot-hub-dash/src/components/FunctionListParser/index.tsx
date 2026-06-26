// src/components/FunctionListParser/index.tsx
import React, { useState } from 'react';
import type { FunctionManifest, OpenSocketFunc } from './types';
import styles from './FunctionListParser.module.css';

interface FunctionListParserProps {
  manifest: FunctionManifest;
  onInvoke: (func: OpenSocketFunc, args: Record<string, string>) => Promise<void>;
  compact?: boolean; // 紧凑模式，用于预览窗格
}

const FunctionListParser: React.FC<FunctionListParserProps> = ({
  manifest,
  onInvoke,
  compact = false,
}) => {
  const [selectedFuncName, setSelectedFuncName] = useState<string | null>(null);
  const [inputValues, setInputValues] = useState<Record<string, string>>({});
  const [loading, setLoading] = useState(false);

  const funcNames = Object.keys(manifest.openSocket);
  const selectedFunc = selectedFuncName ? manifest.openSocket[selectedFuncName] : null;

  const handleArgChange = (argName: string, value: string) => {
    setInputValues(prev => ({ ...prev, [argName]: value }));
  };

  const buildCallArgs = (func: OpenSocketFunc): Record<string, string> => {
    const result: Record<string, string> = {};
    for (const [argName, arg] of Object.entries(func.args)) {
      if (arg.type === 'static') {
        result[argName] = arg.value;
      } else if (arg.type === 'optional') {
        const selected = inputValues[argName];
        if (selected) {
          result[argName] = selected;
        } else if (arg.options.length > 0) {
          result[argName] = arg.options[0][1];
        }
      } else if (arg.type === 'input') {
        result[argName] = inputValues[argName] ?? arg.defaultVal ?? '';
      }
    }
    return result;
  };

  const handleCall = async () => {
    if (!selectedFunc) return;
    const args = buildCallArgs(selectedFunc);
    setLoading(true);
    try {
      await onInvoke(selectedFunc, args);
    } catch (err) {
      console.error(err);
    } finally {
      setLoading(false);
    }
  };

  const renderArg = (argName: string, arg: any) => {
    if (arg.type === 'static') {
      return (
        <div className={styles.argItem} key={argName}>
          <label className={styles.argLabel}>
            {argName}
            {arg.description && <span className={styles.argDesc}>（{arg.description}）</span>}
          </label>
          <div className={styles.argControl}>
            <span className={styles.staticValue}>{arg.value}</span>
          </div>
        </div>
      );
    } else if (arg.type === 'optional') {
      return (
        <div className={styles.argItem} key={argName}>
          <label className={styles.argLabel}>
            {argName}
            {arg.description && <span className={styles.argDesc}>（{arg.description}）</span>}
          </label>
          <div className={styles.argControl}>
            <select
              value={inputValues[argName] || arg.options[0]?.[1] || ''}
              onChange={(e) => handleArgChange(argName, e.target.value)}
            >
              {arg.options.map(([label, value]: [string, string]) => (
                <option key={value} value={value}>{label}</option>
              ))}
            </select>
          </div>
        </div>
      );
    } else if (arg.type === 'input') {
      return (
        <div className={styles.argItem} key={argName}>
          <label className={styles.argLabel}>
            {argName}
            {arg.description && <span className={styles.argDesc}>（{arg.description}）</span>}
          </label>
          <div className={styles.argControl}>
            <input
              type="text"
              value={inputValues[argName] ?? arg.defaultVal ?? ''}
              placeholder={arg.defaultVal || '请输入'}
              onChange={(e) => handleArgChange(argName, e.target.value)}
            />
          </div>
        </div>
      );
    }
    return null;
  };

  if (compact && !selectedFunc) {
    // 紧凑模式下，若未选中任何功能，只显示功能列表
    return (
      <div className={styles.compactContainer}>
        <div className={styles.funcListCompact}>
          {funcNames.map(name => (
            <button
              key={name}
              className={styles.funcChip}
              onClick={() => setSelectedFuncName(name)}
            >
              {name}
            </button>
          ))}
        </div>
        {selectedFunc && (
          <div className={styles.funcDetailCompact}>
            {/* 显示参数和调用按钮，这里简化，但实际可以展开 */}
          </div>
        )}
      </div>
    );
  }

  return (
    <div className={styles.parserContainer}>
      <div className={styles.funcList}>
        <h4>功能列表</h4>
        <ul>
          {funcNames.map(name => (
            <li
              key={name}
              className={selectedFuncName === name ? styles.active : ''}
              onClick={() => {
                setSelectedFuncName(name);
                setInputValues({});
              }}
            >
              {name}
              <span className={styles.funcDesc}>{manifest.openSocket[name].description}</span>
            </li>
          ))}
        </ul>
      </div>

      {selectedFunc && (
        <div className={styles.funcDetail}>
          <h4>参数配置</h4>
          <div className={styles.argsForm}>
            {Object.entries(selectedFunc.args).map(([name, arg]) => renderArg(name, arg))}
          </div>
          <button className={styles.callBtn} onClick={handleCall} disabled={loading}>
            {loading ? '调用中...' : '调用'}
          </button>
          {selectedFunc.returns.length > 0 && (
            <div className={styles.returnInfo}>
              <h5>返回值</h5>
              <ul>
                {selectedFunc.returns.map(([name, desc]) => (
                  <li key={name}>{name}：{desc || '无描述'}</li>
                ))}
              </ul>
            </div>
          )}
        </div>
      )}
    </div>
  );
};

export default FunctionListParser;