// SimplePlayerController.cs
// ─────────────────────────────────────────────────────────────────────────────
// 极简第一人称控制器 — WASD 移动 + 鼠标转向
// 对话时自动锁定移动，释放光标给 UI
// ─────────────────────────────────────────────────────────────────────────────

using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame.Demo
{
    [RequireComponent(typeof(Rigidbody))]
    public class SimplePlayerController : MonoBehaviour
    {
        [Header("移动")]
        [SerializeField] private float moveSpeed = 5f;
        [SerializeField] private float sprintMultiplier = 1.8f;

        [Header("视角")]
        [SerializeField] private float mouseSensitivity = 2f;
        [SerializeField] private float maxLookAngle = 80f;

        [Header("交互")]
        [SerializeField] private KeyCode interactKey = KeyCode.E;
        [SerializeField] private KeyCode toggleCursorKey = KeyCode.Tab;

        // ── 状态 ────────────────────────────────────────────────────────
        public bool IsInDialog { get; set; }  // 外部设，对话时锁定移动

        private Rigidbody _rb;
        private Transform _camTF;
        private float     _rotX;
        private bool      _cursorLocked = true;

        void Start()
        {
            _rb = GetComponent<Rigidbody>();
            _rb.freezeRotation = true;

            _camTF = GetComponentInChildren<Camera>()?.transform;
            if (_camTF == null)
                _camTF = Camera.main?.transform;

            SetCursorLock(true);
        }

        void Update()
        {
            // Tab 切换光标
            if (Input.GetKeyDown(toggleCursorKey))
                SetCursorLock(!_cursorLocked);

            // 对话时不处理移动和视角
            if (IsInDialog) return;

            HandleLook();
        }

        void FixedUpdate()
        {
            if (IsInDialog) return;
            HandleMove();
        }

        // ── 视角 ────────────────────────────────────────────────────────
        void HandleLook()
        {
            if (!_cursorLocked) return;

            float mx = Input.GetAxis("Mouse X") * mouseSensitivity;
            float my = Input.GetAxis("Mouse Y") * mouseSensitivity;

            // 水平旋转整个 Player
            transform.Rotate(Vector3.up, mx);

            // 垂直旋转相机
            _rotX -= my;
            _rotX = Mathf.Clamp(_rotX, -maxLookAngle, maxLookAngle);

            if (_camTF != null)
                _camTF.localRotation = Quaternion.Euler(_rotX, 0, 0);
        }

        // ── 移动 ────────────────────────────────────────────────────────
        void HandleMove()
        {
            float h = Input.GetAxisRaw("Horizontal");
            float v = Input.GetAxisRaw("Vertical");

            Vector3 dir = (transform.forward * v + transform.right * h).normalized;
            float speed = moveSpeed * (Input.GetKey(KeyCode.LeftShift) ? sprintMultiplier : 1f);

            Vector3 vel = dir * speed;
            vel.y = _rb.linearVelocity.y; // 保持重力
            _rb.linearVelocity = vel;
        }

        // ── 光标锁定 ────────────────────────────────────────────────────
        public void SetCursorLock(bool locked)
        {
            _cursorLocked = locked;
            Cursor.lockState = locked ? CursorLockMode.Locked : CursorLockMode.None;
            Cursor.visible = !locked;
        }

        /// <summary>进入对话模式（释放光标 + 禁止移动）</summary>
        public void EnterDialog()
        {
            IsInDialog = true;
            SetCursorLock(false);
            _rb.linearVelocity = Vector3.zero;
        }

        /// <summary>退出对话模式（锁定光标 + 恢复移动）</summary>
        public void ExitDialog()
        {
            IsInDialog = false;
            SetCursorLock(true);
        }
    }
}
