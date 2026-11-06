using System;
using UnityEngine;

namespace Player
{
    /// <summary>
    /// Smooth 2.5D camera controller tailored for Terraria-like side-scrolling sandbox gameplay.
    /// Features:
    /// - Smooth position damping (Vector3.SmoothDamp)
    /// - Dynamic horizontal lookahead in player movement / facing direction
    /// - Configurable 2.5D distance and angle for optimal depth perspective
    /// - Mouse wheel zoom control
    /// - Camera bounds clamping (prevents camera from falling below bedrock)
    /// </summary>
    [DisallowMultipleComponent]
    [RequireComponent(typeof(Camera))]
    public class CameraController2D5 : MonoBehaviour
    {
        [Header("Target")]
        [SerializeField] private Transform target;
        [SerializeField] private Vector3 offset = new Vector3(0f, 1.8f, -16f);

        [Header("Damping & Follow")]
        [SerializeField] private float smoothTimeX = 0.18f;
        [SerializeField] private float smoothTimeY = 0.25f;
        [SerializeField] private float lookaheadDistance = 2.5f;
        [SerializeField] private float lookaheadSmoothSpeed = 4.0f;

        [Header("Zoom Settings")]
        [SerializeField] private float minDistance = 8.0f;
        [SerializeField] private float maxDistance = 32.0f;
        [SerializeField] private float zoomSpeed = 4.0f;

        [Header("World Bounds")]
        [SerializeField] private float minY = 4.0f; // Keep camera above bedrock

        private Camera _camera;
        private PlayerController2D5 _playerController;
        private Vector3 _currentVelocity;
        private float _currentLookaheadX;
        private float _targetDistance;

        public Transform Target
        {
            get => target;
            set => target = value;
        }

        private void Awake()
        {
            _camera = GetComponent<Camera>();
            _targetDistance = Mathf.Abs(offset.z);
        }

        private void Start()
        {
            FindTargetIfMissing();

            if (target != null)
            {
                Vector3 startPos = target.position + offset;
                startPos.z = -_targetDistance;
                transform.position = startPos;
            }
        }

        private void LateUpdate()
        {
            FindTargetIfMissing();
            if (target == null) return;

            HandleZoomInput();
            FollowTarget();
        }

        private void FindTargetIfMissing()
        {
            if (target == null)
            {
                PlayerController2D5 player = FindObjectOfType<PlayerController2D5>();
                if (player != null)
                {
                    target = player.transform;
                    _playerController = player;
                }
            }
            else if (_playerController == null)
            {
                _playerController = target.GetComponent<PlayerController2D5>();
            }
        }

        private void HandleZoomInput()
        {
            // Zoom with mouse scroll wheel if not holding UI modifiers
            float scroll = Input.GetAxis("Mouse ScrollWheel");
            if (Mathf.Abs(scroll) > 0.001f && (Input.GetKey(KeyCode.LeftControl) || Input.GetKey(KeyCode.RightControl)))
            {
                _targetDistance -= scroll * zoomSpeed * 10f;
                _targetDistance = Mathf.Clamp(_targetDistance, minDistance, maxDistance);
            }
        }

        private void FollowTarget()
        {
            // Determine lookahead based on player movement
            float targetLookaheadX = 0f;
            if (_playerController != null)
            {
                targetLookaheadX = _playerController.IsFacingRight ? lookaheadDistance : -lookaheadDistance;
            }

            _currentLookaheadX = Mathf.Lerp(_currentLookaheadX, targetLookaheadX, Time.deltaTime * lookaheadSmoothSpeed);

            // Compute desired target position
            Vector3 targetPos = target.position;
            float desiredX = targetPos.x + offset.x + _currentLookaheadX;
            float desiredY = targetPos.y + offset.y;

            // Clamp minimum Y to stay above bedrock level
            if (desiredY < minY) desiredY = minY;

            // Smooth position damp
            float currentX = Mathf.SmoothDamp(transform.position.x, desiredX, ref _currentVelocity.x, smoothTimeX);
            float currentY = Mathf.SmoothDamp(transform.position.y, desiredY, ref _currentVelocity.y, smoothTimeY);

            transform.position = new Vector3(currentX, currentY, -_targetDistance);
        }
    }
}
