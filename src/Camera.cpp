#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;
        this->cameraFrontDirection = glm::normalize(-cameraPosition + cameraTarget);
        this->cameraRightDirection = glm::normalize(glm::cross(cameraUp, this->cameraFrontDirection));
    } 

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraTarget, cameraUpDirection);
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        switch (direction) {
        case MOVE_FORWARD:
            this->cameraPosition += this->cameraFrontDirection * speed;
            this->cameraTarget += this->cameraFrontDirection * speed;
            break;
        case MOVE_BACKWARD:
            this->cameraPosition -= this->cameraFrontDirection * speed;
            this->cameraTarget -= this->cameraFrontDirection * speed;
            break;
        case MOVE_LEFT:
            this->cameraPosition += this->cameraRightDirection * speed;
            this->cameraTarget += this->cameraRightDirection * speed;
            break;
        case MOVE_RIGHT:
            this->cameraPosition -= this->cameraRightDirection * speed;
            this->cameraTarget -= this->cameraRightDirection * speed;
            break;
        case MOVE_UP:
            this->cameraPosition += this->cameraUpDirection * speed;
            this->cameraTarget += this->cameraUpDirection * speed;
            this->cameraUpDirection += this->cameraUpDirection * speed;
            this->cameraUpDirection = glm::normalize(this->cameraUpDirection);
            break;
        case MOVE_DOWN:
            this->cameraPosition -= this->cameraUpDirection * speed;
            this->cameraTarget -= this->cameraUpDirection * speed;
            this->cameraUpDirection -= this->cameraUpDirection * speed;
            this->cameraUpDirection = glm::normalize(this->cameraUpDirection);
            break;
        }
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {

        this->cameraTarget.x = this->cameraPosition.x + glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
        this->cameraTarget.z = this->cameraPosition.z - glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
        this->cameraTarget.y = this->cameraPosition.y + glm::sin(glm::radians(pitch));

        this->cameraFrontDirection = glm::normalize(-cameraPosition + cameraTarget);
        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraUpDirection, this->cameraFrontDirection));
    }

    void Camera::preview(bool flag, FILE* pf) {
        if (flag) {
            glm::vec3 tmp;
            if (fscanf(pf, "%f %f %f\n", &tmp.x, &tmp.y, &tmp.z) == 3) {
                this->cameraPosition = tmp;
                if (fscanf(pf, "%f %f %f\n", &tmp.x, &tmp.y, &tmp.z) == 3) {
                    this->cameraTarget = tmp;
                }
            }
        }
        else {
            fseek(pf, 0, SEEK_SET);
        }

    }
}