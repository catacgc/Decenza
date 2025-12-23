import QtQuick
import QtQuick.Controls
import QtMultimedia
import DE1App

Page {
    id: screensaverPage
    objectName: "screensaverPage"
    background: Rectangle { color: "black" }

    // Apple TV aerial screensaver videos from Internet Archive (tvOS 10, 1080p)
    // Source: https://archive.org/details/apple-wallpapers-and-screensavers
    readonly property string videoBaseUrl: "https://archive.org/download/apple-wallpapers-and-screensavers/tvOS/tvOS%2010/"
    readonly property var videoFiles: [
        "b1-1.mp4",      // San Francisco
        "b1-2.mp4",      // San Francisco
        "b2-1.mp4",      // New York
        "b2-2.mp4",      // New York
        "b3-1.mp4",      // Hawaii
        "b4-1.mp4",      // China
        "b5-1.mp4",      // London
        "b6-1.mp4",      // Dubai
        "b7-1.mp4",      // Liwa
        "b8-1.mp4",      // Greenland
        "b9-1.mp4",      // Los Angeles
        "b10-1.mp4"      // Hong Kong
    ]

    property int currentVideoIndex: Math.floor(Math.random() * videoFiles.length)
    property int videoFailCount: 0
    property bool videoDisabled: false
    property string lastFailedSource: ""

    Component.onCompleted: {
        // Start with a random video
        currentVideoIndex = Math.floor(Math.random() * videoFiles.length)
        tryPlayVideo()
    }

    function tryPlayVideo() {
        if (videoDisabled || videoFailCount >= videoFiles.length) {
            // All videos failed, give up and use fallback
            console.log("All videos failed, using fallback animation")
            videoDisabled = true
            mediaPlayer.stop()
            return
        }
        var url = videoBaseUrl + videoFiles[currentVideoIndex]
        mediaPlayer.source = url
        mediaPlayer.play()
    }

    function handleVideoFailure() {
        if (videoDisabled) return

        // Prevent handling the same failure twice (both onError and onMediaStatus can fire)
        var currentSource = mediaPlayer.source.toString()
        if (currentSource === lastFailedSource) return
        lastFailedSource = currentSource

        videoFailCount++
        console.log("Video failed (" + videoFailCount + "/" + videoFiles.length + "):", videoFiles[currentVideoIndex])

        if (videoFailCount >= videoFiles.length) {
            console.log("All videos failed, using fallback animation")
            videoDisabled = true
            mediaPlayer.stop()
            return
        }

        currentVideoIndex = (currentVideoIndex + 1) % videoFiles.length
        tryPlayVideo()
    }

    MediaPlayer {
        id: mediaPlayer
        audioOutput: AudioOutput { volume: 0 }  // Muted
        videoOutput: videoOutput

        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.EndOfMedia) {
                // Play next video (reset fail count on success)
                videoFailCount = 0
                lastFailedSource = ""
                currentVideoIndex = (currentVideoIndex + 1) % videoFiles.length
                tryPlayVideo()
            } else if (mediaStatus === MediaPlayer.InvalidMedia ||
                       mediaStatus === MediaPlayer.NoMedia) {
                handleVideoFailure()
            }
        }

        onErrorOccurred: {
            handleVideoFailure()
        }

        onPlaybackStateChanged: {
            // Reset fail count when video starts playing successfully
            if (playbackState === MediaPlayer.PlayingState) {
                videoFailCount = 0
                lastFailedSource = ""
            }
        }
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
        visible: !videoDisabled
    }

    // Fallback: show a subtle animation if video fails
    Rectangle {
        id: fallbackBackground
        anchors.fill: parent
        visible: videoDisabled || mediaPlayer.playbackState !== MediaPlayer.PlayingState
        z: 1  // Above VideoOutput

        // Subtle gradient animation as fallback
        Rectangle {
            id: gradientRect
            anchors.fill: parent
            property real gradientHue: 0.6

            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Qt.hsla(gradientRect.gradientHue, 0.4, 0.15, 1.0)
                }
                GradientStop {
                    position: 1.0
                    color: Qt.hsla((gradientRect.gradientHue + 0.5) % 1.0, 0.4, 0.08, 1.0)
                }
            }

            NumberAnimation on gradientHue {
                from: 0
                to: 1
                duration: 30000
                loops: Animation.Infinite
            }
        }
    }

    // Clock display
    Text {
        z: 2
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 50
        text: Qt.formatTime(currentTime, "hh:mm")
        color: "white"
        opacity: 0.8
        font.pixelSize: 80
        font.weight: Font.Light

        property date currentTime: new Date()

        Timer {
            interval: 1000
            running: true
            repeat: true
            onTriggered: parent.currentTime = new Date()
        }
    }

    // Touch hint (fades out)
    Text {
        id: touchHint
        z: 2
        anchors.centerIn: parent
        text: "Touch to wake"
        color: "white"
        opacity: 0.5
        font.pixelSize: 24

        OpacityAnimator {
            target: touchHint
            from: 0.5
            to: 0
            duration: 3000
            running: true
        }
    }

    // Touch anywhere to wake
    MouseArea {
        z: 3
        anchors.fill: parent
        onClicked: wake()
        onPressed: wake()
    }

    // Also wake on key press
    Keys.onPressed: wake()

    function wake() {
        // Stop video
        mediaPlayer.stop()

        // Wake up the DE1
        if (DE1Device.connected) {
            DE1Device.wakeUp()
        }

        // Try to reconnect to saved scale
        BLEManager.tryDirectConnectToScale()

        // Navigate back to idle
        root.goToIdle()
    }

    // Auto-wake when DE1 wakes up externally (button press on machine)
    Connections {
        target: DE1Device
        function onStateChanged() {
            // If DE1 wakes up externally, wake the app too
            var state = DE1Device.stateString
            if (state !== "Sleep" && state !== "GoingToSleep") {
                mediaPlayer.stop()
                root.goToIdle()
            }
        }
    }
}
