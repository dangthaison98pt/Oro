# meta-description: Classic movement for gravity games (platformer, ...)
using Orostride;

public class _CLASS_ : _BASE_
{
    const float Speed = 300.0f;
    const float JumpVelocity = -400.0f;

    void _physics_process(float delta)
    {
        // Add the gravity.
        if (!is_on_floor())
        {
            velocity += get_gravity() * delta;
        }

        // Handle jump.
        if (Input.is_action_just_pressed("ui_accept") && is_on_floor())
        {
            velocity.y = JumpVelocity;
        }

        // Get the input direction and handle the movement/deceleration.
        float direction = Input.get_axis("ui_left", "ui_right");
        if (direction != 0)
        {
            velocity.x = direction * Speed;
        }
        else
        {
            velocity.x = Mathf.MoveTowards(velocity.x, 0, Speed);
        }

        move_and_slide();
    }
}
