import cx from 'classnames';
import { useTheme } from '../../context/ThemeContext';
import './index.less';

// 星形 SVG 组件（保持不变）
const Star = ({ color = '#fff' }: { color?: string }) => {
  const width = 20;
  const half = 10;   // width/2
  const quat = 5;    // width/4
  return (
    <svg
      version="1.1"
      viewBox={`0 0 ${width} ${width}`}
      height={width}
      width={width}
      style={{ width: '100%', height: '100%' }}
    >
      <path
        d={`M 0 ${half} c ${quat} 0 ${half} -${quat} ${half} -${half} c 0 ${quat} ${quat} ${half} ${half} ${half} c -${quat} 0 -${half} ${quat} -${half} ${half} c 0 -${quat} -${quat} -${half} -${half} -${half}`}
        stroke={color}
        strokeWidth={1}
        fill={color}
      />
    </svg>
  );
};

export default function ThemeToggle() {
  const { theme, toggleTheme } = useTheme();
  const isNight = theme === 'dark';

  return (
    <span className={cx('button', 'theme-toggle-scaled', { night: isNight })} onClick={toggleTheme}>
      <span className={cx('btnInner', { night: isNight })}>
        <span className="circle">
          <span className="circleNight">
            <span className="crater" />
            <span className={cx('crater', 'crater2')} />
            <span className={cx('crater', 'crater3')} />
          </span>
        </span>
        <span className="haloBox">
          <span className="halo" />
          <span className={cx('halo', 'halo2')} />
          <span className={cx('halo', 'halo3')} />
        </span>
        <span className="clouds">
          {Array(7)
            .fill(1)
            .map((_, idx) => (
              <span key={idx} className={cx('cloud', `cloud${idx + 1}`)} />
            ))}
        </span>
        <span className={cx('clouds', 'clouds2')}>
          {Array(7)
            .fill(1)
            .map((_, idx) => (
              <span key={idx} className={cx('cloud', `cloud${idx + 1}`)} />
            ))}
        </span>
        <span className="stars">
          <span className="star">
            <Star />
          </span>
          {Array(10)
            .fill(1)
            .map((_, idx) => (
              <span key={idx} className={cx('star', `star${idx + 2}`)}>
                <Star />
              </span>
            ))}
        </span>
      </span>
    </span>
  );
}