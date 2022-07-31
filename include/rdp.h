/**
 * @file rdp.h
 * @brief Hardware Display Interface
 * @ingroup rdp
 */
#ifndef __LIBDRAGON_RDP_H
#define __LIBDRAGON_RDP_H

#include "display.h"
#include "graphics.h"
#include <stdbool.h>

/**
 * @addtogroup rdp
 * @{
 */

/** @brief DP start register */
#define DP_START      ((volatile uint32_t*)0xA4100000)

/** @brief DP end register */
#define DP_END        ((volatile uint32_t*)0xA4100004)

/** @brief DP current register */
#define DP_CURRENT    ((volatile uint32_t*)0xA4100008)

/** @brief DP status register */
#define DP_STATUS     ((volatile uint32_t*)0xA410000C)

/** @brief DP clock counter */
#define DP_CLOCK      ((volatile uint32_t*)0xA4100010)

/** @brief DP command buffer busy */
#define DP_BUSY       ((volatile uint32_t*)0xA4100014)

/** @brief DP pipe busy */
#define DP_PIPE_BUSY  ((volatile uint32_t*)0xA4100018)

/** @brief DP tmem busy */
#define DP_TMEM_BUSY  ((volatile uint32_t*)0xA410001C)

/** @brief DP is using DMEM DMA */
#define DP_STATUS_DMEM_DMA          (1 << 0)
/** @brief DP is frozen */
#define DP_STATUS_FREEZE            (1 << 1)
/** @brief DP is flushed */
#define DP_STATUS_FLUSH             (1 << 2)
/** @brief DP GCLK is alive */
#define DP_STATUS_GCLK_ALIVE        (1 << 3)
/** @brief DP TMEM is busy */
#define DP_STATUS_TMEM_BUSY         (1 << 4)
/** @brief DP pipeline is busy */
#define DP_STATUS_PIPE_BUSY         (1 << 5)
/** @brief DP command unit is busy */
#define DP_STATUS_BUSY              (1 << 6)
/** @brief DP command buffer is ready */
#define DP_STATUS_BUFFER_READY      (1 << 7)
/** @brief DP DMA is busy */
#define DP_STATUS_DMA_BUSY          (1 << 8)
/** @brief DP command end register is valid */
#define DP_STATUS_END_VALID         (1 << 9)
/** @brief DP command start register is valid */
#define DP_STATUS_START_VALID       (1 << 10)

#define DP_WSTATUS_RESET_XBUS_DMEM_DMA  (1<<0) ///< DP_STATUS write mask: clear #DP_STATUS_DMEM_DMA bit
#define DP_WSTATUS_SET_XBUS_DMEM_DMA    (1<<1) ///< DP_STATUS write mask: set #DP_STATUS_DMEM_DMA bit
#define DP_WSTATUS_RESET_FREEZE         (1<<2) ///< DP_STATUS write mask: clear #DP_STATUS_FREEZE bit
#define DP_WSTATUS_SET_FREEZE           (1<<3) ///< DP_STATUS write mask: set #DP_STATUS_FREEZE bit
#define DP_WSTATUS_RESET_FLUSH          (1<<4) ///< DP_STATUS write mask: clear #DP_STATUS_FLUSH bit
#define DP_WSTATUS_SET_FLUSH            (1<<5) ///< DP_STATUS write mask: set #DP_STATUS_FLUSH bit
#define DP_WSTATUS_RESET_TMEM_COUNTER   (1<<6) ///< DP_STATUS write mask: clear TMEM counter
#define DP_WSTATUS_RESET_PIPE_COUNTER   (1<<7) ///< DP_STATUS write mask: clear PIPE counter
#define DP_WSTATUS_RESET_CMD_COUNTER    (1<<8) ///< DP_STATUS write mask: clear CMD counter
#define DP_WSTATUS_RESET_CLOCK_COUNTER  (1<<9) ///< DP_STATUS write mask: clear CLOCK counter

/**
 * @brief Mirror settings for textures
 */
typedef enum
{
    /** @brief Disable texture mirroring */
    MIRROR_DISABLED,
    /** @brief Enable texture mirroring on x axis */
    MIRROR_X,
    /** @brief Enable texture mirroring on y axis */
    MIRROR_Y,
    /** @brief Enable texture mirroring on both x & y axis */
    MIRROR_XY
} mirror_t;

/**
 * @brief Caching strategy for loaded textures
 */
typedef enum
{
    /** @brief Textures are assumed to be pre-flushed */
    FLUSH_STRATEGY_NONE,
    /** @brief Cache will be flushed on all incoming textures */
    FLUSH_STRATEGY_AUTOMATIC
} flush_t;

/** @} */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the RDP system
 */
void rdp_init( void );

/**
 * @brief Attach the RDP to a surface
 *
 * This function allows the RDP to operate on surfaces, that is memory buffers
 * that can be used as render targets. For instance, it can be used with
 * framebuffers acquired by calling #display_lock, or to render to an offscreen
 * buffer created with #surface_new.
 * 
 * This should be performed before any rendering operations to ensure that the RDP
 * has a valid output buffer to operate on.
 *
 * @param[in] surface
 *            A surface pointer
 *            
 * @see surface_new
 * @see display_lock
 */
void rdp_attach( surface_t *surface );

/**
 * @brief Detach the RDP from the current surface, after the RDP will have
 *        finished writing to it.
 *
 * This function will ensure that all RDP rendering operations have completed
 * before detaching the surface. As opposed to #rdp_detach, this function will
 * not block. An option callback will be called when the RDP has finished drawing
 * and is detached.
 * 
 * @param[in] cb
 *            Optional callback that will be called when the RDP is detached
 *            from the current surface
 * @param[in] arg
 *            Argument to the callback.
 *            
 * @see #rdp_detach
 */
void rdp_detach_async( void (*cb)(void*), void *arg );

/**
 * @brief Detach the RDP from the current surface, after the RDP will have
 *        finished writing to it.
 *
 * This function will ensure that all RDP rendering operations have completed
 * before detaching the surface. As opposed to #rdp_detach_async, this function
 * will block, doing a spinlock until the RDP has finished.
 * 
 * @note This function requires interrupts to be enabled to operate correctly.
 * 
 * @see #rdp_detach_async
 */
void rdp_detach( void );

/**
 * @brief Check if the RDP is currently attached to a surface
 */
bool rdp_is_attached( void );

/**
 * @brief Check if it is currently possible to attach a new display context to the RDP.
 *
 * Since #rdp_detach_display_async will not detach a display context immediately, but asynchronously,
 * it may still be attached when trying to attach the next one. Attempting to attach a display context
 * while another is already attached will lead to an error, so use this function to check whether it
 * is possible first. It will return true if no display context is currently attached, and false otherwise.
 */
#define rdp_can_attach() (!rdp_is_attached())


/**
 * @brief Asynchronously detach the current display from the RDP and automatically call #display_show on it
 *
 * This macro is just a shortcut for `rdp_detach_display_async(display_show)`. Use this if you
 * are done rendering with the RDP and just want to submit the attached display context to be shown without
 * any further postprocessing.
 */
#define rdp_auto_show_display(disp) ({ \
    rdp_detach_async((void(*)(void*))display_show, (disp)); \
})

/**
 * @brief Enable display of 2D filled (untextured) rectangles
 *
 * This must be called before using #rdp_draw_filled_rectangle.
 */
void rdp_enable_primitive_fill( void );

/**
 * @brief Enable display of 2D filled (untextured) triangles
 *
 * This must be called before using #rdp_draw_filled_triangle.
 */
void rdp_enable_blend_fill( void );

/**
 * @brief Enable display of 2D sprites
 *
 * This must be called before using #rdp_draw_textured_rectangle_scaled,
 * #rdp_draw_textured_rectangle, #rdp_draw_sprite or #rdp_draw_sprite_scaled.
 */
void rdp_enable_texture_copy( void );

/**
 * @brief Load a sprite into RDP TMEM
 *
 * @param[in] texslot
 *            The RDP texture slot to load this sprite into (0-7)
 * @param[in] texloc
 *            The RDP TMEM offset to place the texture at
 * @param[in] mirror
 *            Whether the sprite should be mirrored when displaying past boundaries
 * @param[in] sprite
 *            Pointer to sprite structure to load the texture from
 *
 * @return The number of bytes consumed in RDP TMEM by loading this sprite
 */
uint32_t rdp_load_texture( uint32_t texslot, uint32_t texloc, mirror_t mirror, sprite_t *sprite );

/**
 * @brief Load part of a sprite into RDP TMEM
 *
 * Given a sprite with vertical and horizontal slices defined, this function will load the slice specified in
 * offset into texture memory.  This is usefl for treating a large sprite as a tilemap.
 *
 * Given a sprite with 3 horizontal slices and two vertical slices, the offsets are as follows:
 *
 * <pre>
 * *---*---*---*
 * | 0 | 1 | 2 |
 * *---*---*---*
 * | 3 | 4 | 5 |
 * *---*---*---*
 * </pre>
 *
 * @param[in] texslot
 *            The RDP texture slot to load this sprite into (0-7)
 * @param[in] texloc
 *            The RDP TMEM offset to place the texture at
 * @param[in] mirror
 *            Whether the sprite should be mirrored when displaying past boundaries
 * @param[in] sprite
 *            Pointer to sprite structure to load the texture from
 * @param[in] offset
 *            Offset of the particular slice to load into RDP TMEM.
 *
 * @return The number of bytes consumed in RDP TMEM by loading this sprite
 */
uint32_t rdp_load_texture_stride( uint32_t texslot, uint32_t texloc, mirror_t mirror, sprite_t *sprite, int offset );

/**
 * @brief Draw a textured rectangle
 *
 * Given an already loaded texture, this function will draw a rectangle textured with the loaded texture.
 * If the rectangle is larger than the texture, it will be tiled or mirrored based on the* mirror setting
 * given in the load texture command.
 *
 * Before using this command to draw a textured rectangle, use #rdp_enable_texture_copy to set the RDP
 * up in texture mode.
 *
 * @param[in] texslot
 *            The texture slot that the texture was previously loaded into (0-7)
 * @param[in] tx
 *            The pixel X location of the top left of the rectangle
 * @param[in] ty
 *            The pixel Y location of the top left of the rectangle
 * @param[in] bx
 *            The pixel X location of the bottom right of the rectangle
 * @param[in] by
 *            The pixel Y location of the bottom right of the rectangle
 * @param[in] mirror
 *            Whether the texture should be mirrored
 */
void rdp_draw_textured_rectangle( uint32_t texslot, int tx, int ty, int bx, int by,  mirror_t mirror );

/**
 * @brief Draw a textured rectangle with a scaled texture
 *
 * Given an already loaded texture, this function will draw a rectangle textured with the loaded texture
 * at a scale other than 1.  This allows rectangles to be drawn with stretched or squashed textures.
 * If the rectangle is larger than the texture after scaling, it will be tiled or mirrored based on the
 * mirror setting given in the load texture command.
 *
 * Before using this command to draw a textured rectangle, use #rdp_enable_texture_copy to set the RDP
 * up in texture mode.
 *
 * @param[in] texslot
 *            The texture slot that the texture was previously loaded into (0-7)
 * @param[in] tx
 *            The pixel X location of the top left of the rectangle
 * @param[in] ty
 *            The pixel Y location of the top left of the rectangle
 * @param[in] bx
 *            The pixel X location of the bottom right of the rectangle
 * @param[in] by
 *            The pixel Y location of the bottom right of the rectangle
 * @param[in] x_scale
 *            Horizontal scaling factor
 * @param[in] y_scale
 *            Vertical scaling factor
 * @param[in] mirror
 *            Whether the texture should be mirrored
 */
void rdp_draw_textured_rectangle_scaled( uint32_t texslot, int tx, int ty, int bx, int by, double x_scale, double y_scale,  mirror_t mirror );

/**
 * @brief Draw a texture to the screen as a sprite
 *
 * Given an already loaded texture, this function will draw a rectangle textured with the loaded texture.
 *
 * Before using this command to draw a textured rectangle, use #rdp_enable_texture_copy to set the RDP
 * up in texture mode.
 *
 * @param[in] texslot
 *            The texture slot that the texture was previously loaded into (0-7)
 * @param[in] x
 *            The pixel X location of the top left of the sprite
 * @param[in] y
 *            The pixel Y location of the top left of the sprite
 * @param[in] mirror
 *            Whether the texture should be mirrored
 */
void rdp_draw_sprite( uint32_t texslot, int x, int y ,  mirror_t mirror);

/**
 * @brief Draw a texture to the screen as a scaled sprite
 *
 * Given an already loaded texture, this function will draw a rectangle textured with the loaded texture.
 *
 * Before using this command to draw a textured rectangle, use #rdp_enable_texture_copy to set the RDP
 * up in texture mode.
 *
 * @param[in] texslot
 *            The texture slot that the texture was previously loaded into (0-7)
 * @param[in] x
 *            The pixel X location of the top left of the sprite
 * @param[in] y
 *            The pixel Y location of the top left of the sprite
 * @param[in] x_scale
 *            Horizontal scaling factor
 * @param[in] y_scale
 *            Vertical scaling factor
 * @param[in] mirror
 *            Whether the texture should be mirrored
 */
void rdp_draw_sprite_scaled( uint32_t texslot, int x, int y, double x_scale, double y_scale,  mirror_t mirror);

/**
 * @brief Set the blend draw color for subsequent filled primitive operations
 *
 * This function sets the color of all #rdp_draw_filled_triangle operations that follow.
 *
 * @param[in] color
 *            Color to draw primitives in
 */
void rdp_set_blend_color( uint32_t color );

/**
 * @brief Draw a filled rectangle
 *
 * Given a color set with #rdp_set_primitive_color, this will draw a filled rectangle
 * to the screen.  This is most often useful for erasing a buffer before drawing to it
 * by displaying a black rectangle the size of the screen.  This is much faster than
 * setting the buffer blank in software.  However, if you are planning on drawing to
 * the entire screen, blanking may be unnecessary.  
 *
 * Before calling this function, make sure that the RDP is set to primitive mode by
 * calling #rdp_enable_primitive_fill.
 *
 * @param[in] tx
 *            Pixel X location of the top left of the rectangle
 * @param[in] ty
 *            Pixel Y location of the top left of the rectangle
 * @param[in] bx
 *            Pixel X location of the bottom right of the rectangle
 * @param[in] by
 *            Pixel Y location of the bottom right of the rectangle
 */
void rdp_draw_filled_rectangle( int tx, int ty, int bx, int by );

/**
 * @brief Draw a filled triangle
 *
 * Given a color set with #rdp_set_blend_color, this will draw a filled triangle
 * to the screen. Vertex order is not important.
 *
 * Before calling this function, make sure that the RDP is set to blend mode by
 * calling #rdp_enable_blend_fill.
 *
 * @param[in] x1
 *            Pixel X1 location of triangle
 * @param[in] y1
 *            Pixel Y1 location of triangle
 * @param[in] x2
 *            Pixel X2 location of triangle
 * @param[in] y2
 *            Pixel Y2 location of triangle
 * @param[in] x3
 *            Pixel X3 location of triangle
 * @param[in] y3
 *            Pixel Y3 location of triangle
 */
void rdp_draw_filled_triangle( float x1, float y1, float x2, float y2, float x3, float y3 );

/**
 * @brief Set the flush strategy for texture loads
 *
 * If textures are guaranteed to be in uncached RDRAM or the cache
 * is flushed before calling load operations, the RDP can be told
 * to skip flushing the cache.  This affords a good speedup.  However,
 * if you are changing textures in memory on the fly or otherwise do
 * not want to deal with cache coherency, set the cache strategy to
 * automatic to have the RDP flush cache before texture loads.
 *
 * @param[in] flush
 *            The cache strategy, either #FLUSH_STRATEGY_NONE or
 *            #FLUSH_STRATEGY_AUTOMATIC.
 */
void rdp_set_texture_flush( flush_t flush );

/**
 * @brief Close the RDP system
 *
 * This function closes out the RDP system and cleans up any internal memory
 * allocated by #rdp_init.
 */
void rdp_close( void );


/// @cond

typedef enum
{
    SYNC_FULL,
    SYNC_PIPE,
    SYNC_LOAD,
    SYNC_TILE
} sync_t;

__attribute__((deprecated("use rdp_attach instead")))
static inline void rdp_attach_display( display_context_t disp )
{
    rdp_attach(disp);
}

__attribute__((deprecated("use rdp_detach instead")))
static inline void rdp_detach_display( void )
{
    rdp_detach();
}

__attribute__((deprecated("use rdpq_set_scissor instead")))
void rdp_set_clipping( uint32_t tx, uint32_t ty, uint32_t bx, uint32_t by );

__attribute__((deprecated("default clipping is activated automatically during rdp_attach_display")))
void rdp_set_default_clipping( void );

__attribute__((deprecated("syncs are now performed automatically -- or use rdpq_sync_* functions otherwise")))
void rdp_sync( sync_t sync );

static inline __attribute__((deprecated("use rdpq_set_fill_color instead")))
void rdp_set_primitive_color(uint32_t color) {
    extern void __rdpq_set_fill_color(uint32_t);
    __rdpq_set_fill_color(color);
}

/// @endcond


#ifdef __cplusplus
}
#endif

#endif
