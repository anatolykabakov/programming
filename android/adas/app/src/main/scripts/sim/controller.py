"""
Simple vehicle controller
"""


class SimpleController:
    """Simple vehicle control - drive straight"""

    def __init__(self, desired_speed=20.0):
        self.desired_speed = desired_speed

    def get_control(self, speed, detected_lines=None):
        """Compute throttle and steering - keep going straight"""
        # Maintain desired speed
        throttle = 0.5 if speed < self.desired_speed else 0.0

        # Always drive straight (no steering correction)
        steer = 0.0
        brake = 0.0

        return [steer, throttle, brake]
