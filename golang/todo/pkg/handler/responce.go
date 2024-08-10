package handler

import (
	"github.com/sirupsen/logrus"
	"github.com/gin-gonic/gin"
)

type errorResponce struct {
	Message string `json:"message"`
}

func newErrorResponce(c *gin.Context, statusCode int, message string) {
	logrus.Error(message)
	c.AbortWithStatusJSON(statusCode, errorResponce{message})
}
