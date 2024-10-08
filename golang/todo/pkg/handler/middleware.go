package handler

import (
	"github.com/gin-gonic/gin"
	"net/http"
	"strings"
	"errors"
)

const (
	authorizationHeader = "Authorization"
	userCtx             = "userId"
)

func (h *Handler) userIdentity(c *gin.Context) {
	header := c.GetHeader(authorizationHeader)
	if header == "" {
		newErrorResponce(c, http.StatusUnauthorized, "empty auth header")
		return
	}

	headerParts := strings.Split(header, " ")

	if len(headerParts) != 2 {
		newErrorResponce(c, http.StatusUnauthorized, "invalid auth header")
		return
	}

	userId, err := h.services.Authorization.ParseToken(headerParts[1])
	if err != nil {
		newErrorResponce(c, http.StatusUnauthorized, err.Error())
		return
	}

	c.Set(userCtx, userId)
}

func getUserId(c *gin.Context) (int, error) {
	id, ok := c.Get(userCtx)
	if !ok {
		newErrorResponce(c, http.StatusInternalServerError, "user id not found")
		return 0, errors.New("user id not found")
	}

  userId, ok := id.(int)
	if !ok {
		newErrorResponce(c, http.StatusInternalServerError, "user id is invalide type")
		return 0, errors.New("user id is invalide type")
	}
	return userId, nil
}
