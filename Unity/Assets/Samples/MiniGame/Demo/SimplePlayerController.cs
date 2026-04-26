// SimplePlayerController.cs
// ─────────────────────────────────────────────────────────────────────────────
// 第三人称控制器 — 竖屏小游戏风格
// WASD 移动，鼠标右键/拖拽旋转相机，有角度范围限制
// ─────────────────────────────────────────────────────────────────────────────

using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame.Demo
{
    [RequireComponent(typeof(CharacterController))]
    public class SimplePlayerController : MonoBehaviour
    {
        [Header("移动")]
        [SerializeField] private float moveSpeed = 5f;
        [SerializeField] private float sprintMultiplier = 1.6f;
        [SerializeField] private float gravity = -15f;
        [SerializeField] private float turnSmooth = 0.12f;

        [Header("相机")]
        [SerializeField] private float camDistance = 6f;         // 离玩家距离
        [SerializeField] private float camHeightOffset = 1.5f;   // 注视点偏移（玩家头部）
        [SerializeField] private float camFollowSpeed = 10f;

        [Header("相机旋转（鼠标右键拖拽）")]
        [SerializeField] private float rotateSensitivity = 3f;
        [SerializeField] private float minPitch = 15f;           // 最低俯角
        [SerializeField] private float maxPitch = 70f;           // 最高俯角
        [SerializeField] private float defaultPitch = 35f;       // 默认俯角
        [SerializeField] private float defaultYaw = 0f;

        [Header("相机缩放（滚轮）")]
        [SerializeField] private float zoomSpeed = 2f;
        [SerializeField] private float minDistance = 3f;
        [SerializeField] private float maxDistance = 12f;

        // ── 状态 ────────────────────────────────────────────────────────
        public bool IsInDialog { get; set; }

        private CharacterController _cc;
        private Transform _camTF;
        private float     _verticalVel;
        private float     _turnSmoothVel;

        // 相机旋转状态
        private float _camYaw;
        private float _camPitch;
        private float _curDistance;

        void Start()
        {
            _cc = GetComponent<CharacterController>();

            _camTF = GetComponentInChildren<Camera>()?.transform;
            if (_camTF == null && Camera.main != null)
                _camTF = Camera.main.transform;

            // 相机脱离玩家（改为脚本控制跟随）
            if (_camTF != null && _camTF.parent == transform)
                _camTF.SetParent(null);

            _camYaw = defaultYaw;
            _camPitch = defaultPitch;
            _curDistance = camDistance;

            Cursor.lockState = CursorLockMode.None;
            Cursor.visible = true;
        }

        void Update()
        {
            HandleCameraInput();

            if (IsInDialog) return;
            HandleMove();
        }

        void LateUpdate()
        {
            HandleCamera();
        }

        // ── 移动（WASD，方向相对于相机朝向） ────────────────────────────
        void HandleMove()
        {
            float h = Input.GetAxisRaw("Horizontal");
            float v = Input.GetAxisRaw("Vertical");
            Vector3 input = new Vector3(h, 0, v).normalized;

            // 重力
            if (_cc.isGrounded && _verticalVel < 0)
                _verticalVel = -2f;
            _verticalVel += gravity * Time.deltaTime;

            if (input.magnitude >= 0.1f)
            {
                // 移动方向相对于相机水平朝向
                float targetAngle = Mathf.Atan2(input.x, input.z) * Mathf.Rad2Deg + _camYaw;
                float angle = Mathf.SmoothDampAngle(transform.eulerAngles.y, targetAngle, ref _turnSmoothVel, turnSmooth);
                transform.rotation = Quaternion.Euler(0, angle, 0);

                float speed = moveSpeed * (Input.GetKey(KeyCode.LeftShift) ? sprintMultiplier : 1f);
                Vector3 moveDir = Quaternion.Euler(0, targetAngle, 0) * Vector3.forward;
                _cc.Move((moveDir * speed + Vector3.up * _verticalVel) * Time.deltaTime);
            }
            else
            {
                _cc.Move(Vector3.up * _verticalVel * Time.deltaTime);
            }
        }

        // ── 相机输入（右键拖拽旋转 + 滚轮缩放） ────────────────────────
        void HandleCameraInput()
        {
            // 右键拖拽旋转（PC），或双指也行（手机端另做）
            if (Input.GetMouseButton(1))
            {
                _camYaw   += Input.GetAxis("Mouse X") * rotateSensitivity;
                _camPitch -= Input.GetAxis("Mouse Y") * rotateSensitivity;
                _camPitch  = Mathf.Clamp(_camPitch, minPitch, maxPitch);
            }

            // 滚轮缩放
            float scroll = Input.GetAxis("Mouse ScrollWheel");
            if (Mathf.Abs(scroll) > 0.01f)
            {
                _curDistance -= scroll * zoomSpeed;
                _curDistance = Mathf.Clamp(_curDistance, minDistance, maxDistance);
            }
        }

        // ── 相机跟随（轨道式） ──────────────────────────────────────────
        void HandleCamera()
        {
            if (_camTF == null) return;

            // 注视点 = 玩家位置 + 高度偏移
            Vector3 lookAt = transform.position + Vector3.up * camHeightOffset;

            // 球面坐标 → 相机位置
            Quaternion rot = Quaternion.Euler(_camPitch, _camYaw, 0);
            Vector3 offset = rot * new Vector3(0, 0, -_curDistance);
            Vector3 targetPos = lookAt + offset;

            // 平滑跟随
            _camTF.position = Vector3.Lerp(_camTF.position, targetPos, camFollowSpeed * Time.deltaTime);
            _camTF.LookAt(lookAt);
        }

        // ── 对话模式 ────────────────────────────────────────────────────
        public void EnterDialog()
        {
            IsInDialog = true;
        }

        public void ExitDialog()
        {
            IsInDialog = false;
        }
    }
}
